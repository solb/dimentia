#include "GenerateEquations.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/Support/Format.h>
#include <llvm/Support/raw_ostream.h>

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
using std::pair;
using std::string;
using std::vector;

#include "TraceVariables.h"

char GenerateEquations::ID = 0;

GenerateEquations::GenerateEquations() :
  FunctionPass(ID),
  vars(nullptr),
  idxToVal(),
  valToIdx() {}

void GenerateEquations::getAnalysisUsage(AnalysisUsage &info) const {
  info.addRequired<TraceVariables>();
}

bool GenerateEquations::runOnFunction(Function &fun) {
  if(!vars) {
    vars = &getAnalysis<TraceVariables>();
    assert(!idxToVal.size());
    idxToVal.reserve(vars->size());
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

            elem(eqn, idx(*op)) = !subsequent ? -1 : subtrahend;
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

bool GenerateEquations::doFinalization(Module &mod) {
  idx_type cols = idxToVal.size();
  for(vector<int> &row : eqns) {
    assert(row.size() <= cols);
    row.resize(cols);
  }

  vector<string> reprs(cols);
  transform(idxToVal.begin(), idxToVal.end(), reprs.begin(), [this](Value *var){return describeVar(*var);});
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
  if(valToIdx.count(&val))
    return valToIdx[&val];

  idx_type ix = idxToVal.size();
  valToIdx.emplace(&val, ix);
  idxToVal.push_back(&val);
  return ix;
}

Value *GenerateEquations::val(GenerateEquations::idx_type idx) const {
  return idxToVal[idx];
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
  } else
    stm << "(tmp)";

  stm.flush();
  return str;
}

static RegisterPass<GenerateEquations> X("geneqns", "Generate Equations", true, true);
