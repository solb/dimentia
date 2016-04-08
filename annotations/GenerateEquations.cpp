#include "GenerateEquations.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/Support/raw_ostream.h>

using llvm::AnalysisUsage;
using llvm::BasicBlock;
using llvm::DIVariable;
using llvm::Function;
using llvm::Instruction;
using llvm::outs;
using llvm::raw_string_ostream;
using llvm::RegisterPass;
using llvm::Use;
using llvm::Value;
using std::string;
using std::stringstream;

#include "TraceVariables.h"

char GenerateEquations::ID = 0;

GenerateEquations::GenerateEquations() :
  FunctionPass(ID), vars(nullptr) {}

void GenerateEquations::getAnalysisUsage(AnalysisUsage &info) const {
  info.addRequired<TraceVariables>();
}

bool GenerateEquations::runOnFunction(Function &fun) {
  if(!vars)
    vars = &getAnalysis<TraceVariables>();

  for(BasicBlock &block : fun.getBasicBlockList())
    for(Instruction &inst : block.getInstList())
      if(inst.getNumOperands() > 1) {
        const char *operation = " - ";

        switch(inst.getOpcode()) {
        case Instruction::Store: {
          outs() << inst << '\n';
          if(inst.getOperand(0) != inst.getOperand(1))
            outs() << "deg(" << describeVar(*inst.getOperand(1)) << ") = deg(" << describeVar(*inst.getOperand(0)) << ")\n";
          break;
        }

        case Instruction::Add:
        case Instruction::FAdd:
        case Instruction::Sub:
        case Instruction::FSub: {
          outs() << inst << '\n';
          for(Use &op : inst.operands())
            if((*vars)[*op] != (*vars)[inst])
              outs() << "deg(" << describeVar(inst) << ") = deg(" << describeVar(*op) << ")\n";
          break;
        }

        case Instruction::Mul:
        case Instruction::FMul:
          operation = " + ";
        case Instruction::UDiv:
        case Instruction::SDiv:
        case Instruction::FDiv: {
          outs() << inst << '\n';
          outs() << "deg(" << describeVar(inst) << ")";
          bool lhs = true;
          for(Use &op : inst.operands()) {
            outs() << (lhs ? " = " : operation) << "deg(" << describeVar(*op) << ')';
            lhs = false;
          }
          outs() << '\n';
          break;
        }
        }
      }
  return false;
}

string GenerateEquations::describeVar(Value &val) const {
  string str;
  raw_string_ostream stm(str);

  DIVariable *var = (*vars)[val];
  if(var) {
    //stm << var->getFile()->getFilename();
    string scope = var->getScope()->getName();
    if(scope.size())
      stm << scope << "::";
    stm << var->getName();
  } else
    stm << "UNKNOWN (" << val << ')';

  stm.flush();
  return str;
}

static RegisterPass<GenerateEquations> X("geneqns", "Generate Equations", true, true);
