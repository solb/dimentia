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
  std::vector<std::vector<int>> eqns;

public:
  GenerateEquations();

  void getAnalysisUsage(llvm::AnalysisUsage &) const override;

  bool runOnFunction(llvm::Function &) override;

private:
  static int &elem(std::vector<int> &, std::vector<int>::size_type);

  idx_type idx(llvm::Value &);
  llvm::Value *val(idx_type) const;

  std::string describeVar(llvm::Value &) const;
};

#endif
