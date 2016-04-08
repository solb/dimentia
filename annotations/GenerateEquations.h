#ifndef GENERATE_EQUATIONS_H_
#define GENERATE_EQUATIONS_H_

#include <llvm/Pass.h>
#include <string>
#include <unordered_map>

namespace llvm {
class Value;
}

class TraceVariables;

class GenerateEquations : public llvm::FunctionPass {
public:
  static char ID;

private:
  typedef std::vector<llvm::Value *>::size_type idx_type;

  const TraceVariables *vars;
  std::unordered_map<llvm::Value *, idx_type> valToIdx;
  std::vector<llvm::Value *> idxToVal;

public:
  GenerateEquations();

  void getAnalysisUsage(llvm::AnalysisUsage &) const override;

  bool runOnFunction(llvm::Function &) override;

private:
  std::string describeVar(llvm::Value &) const;
};

#endif
