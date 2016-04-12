#include "GenerateEquations.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/raw_ostream.h>

#include "TraceVariables.h"

////////////////////////// TODO: remove this!
#include <lapacke.h>

using llvm::AnalysisUsage;
using llvm::BasicBlock;
using llvm::DIVariable;
using llvm::format_decimal;
using llvm::Function;
using llvm::Instruction;
using llvm::Module;
using llvm::outs;
using llvm::raw_string_ostream;
using llvm::RegisterPass;
using llvm::Use;
using llvm::Value;
using std::hash;
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;
using std::min;

namespace geneqns {
namespace compare {
template<>
class equals<Value *> {
private:
  const TraceVariables &vs;

public:
  explicit equals(const TraceVariables &vs) : vs(vs) {}
  bool operator()(Value *x, Value *y) const {return vs[*x] == vs[*y];}
};

template<>
class hashes<Value *> {
private:
  const TraceVariables &vs;
  const hash<DIVariable *> hs;

public:
  explicit hashes(const TraceVariables &vs) : vs(vs), hs() {}
  size_t operator()(Value *x) const {return hs(vs[*x]);}
};
}
}

using geneqns::compare::equals;
using geneqns::compare::hashes;

#define BUCKET_FACTOR 2

char GenerateEquations::ID = 0;

GenerateEquations::GenerateEquations() :
  FunctionPass(ID),
  vars(nullptr),
  valToIdx(nullptr),
  idxToVal(nullptr) {}

GenerateEquations::~GenerateEquations() {
  if(idxToVal)
    delete idxToVal;
  if(valToIdx)
    delete valToIdx;
}

void GenerateEquations::getAnalysisUsage(AnalysisUsage &info) const {
  info.addRequired<TraceVariables>();
}

bool GenerateEquations::runOnFunction(Function &fun) {
  if(!vars) {
    vars = &getAnalysis<TraceVariables>();
    equals<Value *> vars_eq(*vars);
    hashes<Value *> vars_hs(*vars);
    valToIdx = new unordered_map<Value *, idx_type, hashes<Value *>, equals<Value *>>(BUCKET_FACTOR * vars->uniq(), vars_hs, vars_eq);
    idxToVal = new vector<Value *>();
    idxToVal->reserve(vars->uniq());
    for(const pair<Value *, DIVariable &> &mapping : *vars)
      idx(*mapping.first);
  }

  for(BasicBlock &block : fun.getBasicBlockList())
    for(Instruction &inst : block.getInstList())
      if(inst.getNumOperands() > 1) {
        const char *operation = " - ";
        int subtrahend = 1;

        switch(inst.getOpcode()) {
        case Instruction::Store: {
          outs() << inst << '\n';

          if(inst.getOperand(0) != inst.getOperand(1)) {
            outs() << "deg(" << describeVar(*inst.getOperand(1)) << ") = deg(" << describeVar(*inst.getOperand(0)) << ")\n";

            vector<int> eqn;
            elem(eqn, idx(*inst.getOperand(1))) = 1;
            elem(eqn, idx(*inst.getOperand(0))) = -1;
            eqns.push_back(move(eqn));
          }
          break;
        }

        case Instruction::Add:
        case Instruction::FAdd:
        case Instruction::Sub:
        case Instruction::FSub: {
          outs() << inst << '\n';

          for(Use &op : inst.operands())
            if((*vars)[*op] != (*vars)[inst]) {
              outs() << "deg(" << describeVar(inst) << ") = deg(" << describeVar(*op) << ")\n";

              vector<int> eqn;
              elem(eqn, idx(inst)) = 1;
              elem(eqn, idx(*op)) = -1;
              eqns.push_back(move(eqn));
            }

          break;
        }

        case Instruction::Mul:
        case Instruction::FMul:
          operation = " + ";
          subtrahend = -1;
        case Instruction::UDiv:
        case Instruction::SDiv:
        case Instruction::FDiv: {
          outs() << inst << '\n';
          vector<int> eqn;
          elem(eqn, idx(inst)) = 1;

          outs() << "deg(" << describeVar(inst) << ")";
          bool subsequent = false;
          for(Use &op : inst.operands()) {
            outs() << (!subsequent ? " = " : operation) << "deg(" << describeVar(*op) << ')';

            elem(eqn, idx(*op)) += !subsequent ? -1 : subtrahend;
            subsequent = true;
          }
          outs() << '\n';

          eqns.push_back(move(eqn));
          break;
        }
        }
      }
  return false;
}

vector<int> calcDimensionless(vector<std::vector<int>> eqns) {
  int rows = eqns.size();
  int cols = eqns[0].size();

  char cN = 'N';
  char cA = 'A';

  double* A = new double[rows*cols];
  double* sigmas = new double[rows+cols];
  int work_sz = (rows+cols)*30;
  double* work = new double[work_sz];
  double* Vt = new double[cols*cols];
  int ldvt = cols;
  int info;

  for (int i = 0; i < rows; ++i)
    for (int j = 0; j < cols; ++j)
      A[i+j*rows] = eqns[i][j];

  dgesvd_(&cN, &cA, &rows, &cols, A, &rows,
          sigmas, NULL, &rows, Vt, &ldvt, work, &work_sz, &info);
  assert(info == 0);

  for (int i = min(cols, rows); i < cols; ++i)
    sigmas[i] = 0;

  const double eps = 1e-9;
  vector<int> dimensionless;
  for (int j = 0; j < cols; ++j) {
    bool good = false;
    for (int i = 0; i < cols; ++i) {
      if (fabs(sigmas[i]) < eps && fabs(Vt[i+j*cols]) > eps) {
        good = true;
      }
    }
    if (!good) {
      dimensionless.push_back(j);
      printf("dimensionless %d\n", j);
    }
  }

  for (int i = 0; i < cols; ++i) {
    for (int j = 0; j < cols; ++j)
      printf("%5.2lf ", Vt[i+cols*j]);
    printf("\n");
  }

  delete[] A;
  delete[] sigmas;
  delete[] work;
  delete[] Vt;

  return dimensionless;
}

bool GenerateEquations::doFinalization(Module &mod) {
  idx_type cols = idxToVal->size();
  for(vector<int> &row : eqns) {
    assert(row.size() <= cols);
    row.resize(cols);
  }

  calcDimensionless(eqns);
  // add shit here

  vector<string> reprs(cols);
  transform(idxToVal->begin(), idxToVal->end(), reprs.begin(), [this](Value *var){return describeVar(*var);});
  for_each(reprs.begin(), reprs.end(), [this](string &desc){outs() << desc << ' ';});
  outs() << '\n';
  for(vector<int> &row : eqns) {
    for(vector<int>::size_type ix = 0, sz = row.size(); ix < sz; ++ix)
      outs() << format_decimal(row[ix], reprs[ix].size()) << ' ';
    outs() << '\n';
  }
  return false;
}

int &GenerateEquations::elem(std::vector<int> &v, std::vector<int>::size_type i) {
  if(i >= v.size())
    v.resize(i + 1);
  return v[i];
}

GenerateEquations::idx_type GenerateEquations::idx(Value &val) {
  if(valToIdx->count(&val))
    return (*valToIdx)[&val];

  idx_type ix = idxToVal->size();
  valToIdx->emplace(&val, ix);
  idxToVal->push_back(&val);
  return ix;
}

Value *GenerateEquations::val(GenerateEquations::idx_type idx) const {
  return (*idxToVal)[idx];
}

string GenerateEquations::describeVar(Value &val) const {
  string str;
  raw_string_ostream stm(str);

  DIVariable *var = (*vars)[val];
  if(DIVariable *var = (*vars)[val]) {
    string scope = var->getScope()->getName();
    if(scope.size())
      stm << scope << "::";
    stm << var->getName();
  } else {
    stm << "(tmp:";
    val.printAsOperand(stm, false);
    stm << ')';
  }

  stm.flush();
  return str;
}

static RegisterPass<GenerateEquations> X("geneqns", "Generate Equations", true, true);
