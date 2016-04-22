#include "DimensionalAnalysis.h"

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/raw_ostream.h>
#include <lapacke.h>

#include "TraceVariablesNg.h"

using namespace llvm;
using std::min;
using std::move;
using std::string;
using std::vector;

static string val_str(const Value &obj) {
  string res;
  raw_string_ostream stm(res);
  obj.printAsOperand(stm, false);
  stm.flush();
  return res;
}

dimens_var::dimens_var(const void *hash, string &&str, bool constant) :
    hash((unsigned long) hash),
    str(move(str)),
    constant(constant) {}

dimens_var::dimens_var(const DIVariable &var) :
    dimens_var(&var,
        TraceVariablesNg::str(var)) {}

dimens_var::dimens_var(Value &val) :
    dimens_var(&val,
        val_str(val),
        isa<Constant>(&val)) {}

dimens_var::~dimens_var() = default;

bool dimens_var::operator==(const dimens_var &other) const {
  return this->hash == other.hash;
}

dimens_var::operator unsigned long() const {
  return hash;
}

dimens_var::operator const string &() const {
  return str;
}

bool dimens_var::isa_constant() const {
  return constant;
}

char DimensionalAnalysis::ID = 0;

DimensionalAnalysis::DimensionalAnalysis() :
    ModulePass(ID),
    variables(),
    indices(),
    equations(),
    dimensionless(),
    groupings(nullptr) {}

void DimensionalAnalysis::getAnalysisUsage(llvm::AnalysisUsage &info) const {
  info.addRequired<TraceVariablesNg>();
}

bool DimensionalAnalysis::runOnModule(llvm::Module &module) {
  // We cannot allow modification of this structure or the (parallel) indices won't be stable!
  const TraceVariablesNg &groupings = getAnalysis<TraceVariablesNg>();
  this->groupings = &groupings;

  // Indices less than groupings.vals.size() correspond to source variables.
  variables.reserve(groupings.vals.size());
  for(auto mapping : groupings.vals)
    insert(*mapping.first);
  // From now on, whenever we encounter a new temporary, we'll insert() it, assigning it a larger index.

  for(auto revmap : groupings.vars)
    if(revmap.second.size() > 1)
      for(auto lhs = revmap.second.begin(), end = revmap.second.end(); lhs != end; ++lhs) {
        auto rhs = lhs;
        for(++rhs; rhs != end; ++rhs) {
          errs() << "Source variable analysis revealed that:";
          instruction_setequal(**lhs, **rhs);
        }
      }

  // Process the program's instructions.
  for(Function &function : module)
    for(BasicBlock &block : function.getBasicBlockList())
      for(Instruction &inst : block.getInstList())
        instruction_opdecode(inst);

  // Smooth the equations matrix's jagged boundaries.
  index_type cols = variables.size();
  for(vector<int> &row : equations) {
    assert(row.size() <= cols);
    row.resize(cols);
  }

  // Perform the actual dimensionality calculations.
  calcDimensionless();
  return false;
}

void DimensionalAnalysis::print(llvm::raw_ostream &stream, const llvm::Module *module) const {
  // Here's the matrix we sent to the solver.
  for(const dimens_var &each : variables)
    stream << (const string &) each << ' ';
  stream << '\n';
  for(const vector<int> &row : equations) {
    for(index_type ix = 0, sz = row.size(); ix < sz; ++ix)
      stream << format_decimal(row[ix], ((const string &) variables[ix]).size()) << ' ';
    stream << '\n';
  }

  // And the "winners" are...
  stream << "Found " << dimensionless.size() << " dimensionless variables:\n";
  for(int index : dimensionless)
    stream << (const string &) variables[index] << '\n';
}

void DimensionalAnalysis::calcDimensionless() {
  int rows = equations.size();
  int cols = equations[0].size();

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
      A[i+j*rows] = equations[i][j];

  dgesvd_(&cN, &cA, &rows, &cols, A, &rows,
          sigmas, NULL, &rows, Vt, &ldvt, work, &work_sz, &info);
  assert(info == 0);

  for (int i = min(cols, rows); i < cols; ++i)
    sigmas[i] = 0;

  const double eps = 1e-9;
  for (int j = 0; j < cols; ++j) {
    bool good = false;
    for (int i = 0; i < cols; ++i) {
      if (fabs(sigmas[i]) < eps && fabs(Vt[i+j*cols]) > eps) {
        good = true;
      }
    }
    if (!good) {
      dimensionless.push_back(j);
    }
  }

  delete[] A;
  delete[] sigmas;
  delete[] work;
  delete[] Vt;
}

void DimensionalAnalysis::instruction_opdecode(Instruction &inst) {
  int multiplier = 1;
  switch(inst.getOpcode()) {
    case Instruction::Add:
    case Instruction::FAdd:
    case Instruction::Sub:
    case Instruction::FSub:
    case Instruction::ICmp:
    case Instruction::FCmp:
    case Instruction::PHI:
      errs() << "Processing instruction: " << inst << '\n';
      for(Use &op : inst.operands())
        instruction_setequal(inst, *op);
      break;

    case Instruction::Mul:
    case Instruction::FMul:
    multiplier = -1;
    case Instruction::UDiv:
    case Instruction::SDiv:
    case Instruction::FDiv:
      errs() << "Processing instruction: " << inst << '\n';
      instruction_setadditive(inst, multiplier);
      break;

    case Instruction::Load:
      errs() << "Processing instruction: " << inst << '\n';
      instruction_setequal(inst, *inst.getOperand(0));
      break;

    case Instruction::Store:
      errs() << "Processing instruction: " << inst << '\n';
      instruction_setequal(*inst.getOperand(1), *inst.getOperand(0));
      break;
  }
}

void DimensionalAnalysis::instruction_setequal(const dimens_var &dest, const dimens_var &src) {
  if(src.isa_constant() || dest.isa_constant())
    return;

  errs() << "\tdeg(" << (const string &) dest << ") = deg(" << (const string &) src << ")\n";
  vector<int> equation;
  elem(equation, index(dest)) = 1;
  elem(equation, index(src)) = -1;
  equations.push_back(move(equation));
}

void DimensionalAnalysis::instruction_setadditive(llvm::Instruction &line, int multiplier) {
  if(isa<Constant>(line))
    return;

  bool ran = false;
  vector<int> equation;
  dimens_var lhs = line;
  elem(equation, index(lhs)) = 1;
  for(Use &op : line.operands())
    if(!isa<Constant>(*op)) {
      dimens_var term = *op;
      if(!ran) {
        // First term...
        errs() << "\tdeg(" << (const string &) lhs << ") = deg(" << (const string &) term << ')';
        // is always positive.
        elem(equation, index(term)) = -1;
        ran = true;
      } else
        // Subsequent term
        errs() << (multiplier < 0 ? " + " : " - ") << "deg(" << (const string &) term << ')';
        elem(equation, index(term)) = multiplier;
    }

  if(ran)
    errs() << '\n';
    equations.push_back(move(equation));
}

int &DimensionalAnalysis::elem(vector<int> &arr, DimensionalAnalysis::index_type idx) {
  if(idx >= arr.size())
    arr.resize(idx + 1);
  return arr[idx];
}

DimensionalAnalysis::index_type DimensionalAnalysis::index(const dimens_var &var) {
  return indices.count(var) ? indices[var] : insert(var);
}

DimensionalAnalysis::index_type DimensionalAnalysis::insert(const dimens_var &var) {
  assert(!indices.count(var));

  index_type ind = variables.size();
  variables.push_back(var);
  indices.emplace(var, indices.size());
  assert(variables.size() == indices.size());

  return ind;
}

static RegisterPass<DimensionalAnalysis> dimens("dimens", "Dimensional Analysis", true, true);
