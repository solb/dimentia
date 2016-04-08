#ifndef GENERATE_EQUATIONS_H_
#define GENERATE_EQUATIONS_H_

#include <llvm/Pass.h>
#include <string>

namespace llvm {
class Value;
}

class TraceVariables;

class GenerateEquations : public llvm::FunctionPass {
public:
  static char ID;

private:
  const TraceVariables *vars;

public:
  GenerateEquations();

  void getAnalysisUsage(llvm::AnalysisUsage &) const override;

  bool runOnFunction(llvm::Function &) override;


private:
  std::string describeVar(llvm::Value &) const;
};

#endif
