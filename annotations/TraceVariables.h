#ifndef TRACE_VARIABLES_H_
#define TRACE_VARIABLES_H_

#include <llvm/Pass.h>
#include <unordered_map>

namespace llvm {
class DbgInfoIntrinsic;
class DIGlobalVariable;
class DILocalVariable;
class DIVariable;
class Value;
}

class TraceVariables : public llvm::FunctionPass {
public:
  static char ID;

private:
  std::unordered_map<llvm::Value *, llvm::DIGlobalVariable &> globals;
  std::unordered_map<llvm::Value *, llvm::DILocalVariable &> locals;

public:
  TraceVariables();

  bool doInitialization(llvm::Module &) override;
  bool runOnFunction(llvm::Function &) override;

  llvm::DIVariable *operator[](llvm::Value &);

private:
  static llvm::Value *valOf(llvm::DbgInfoIntrinsic &);
  static llvm::DILocalVariable *varOf(llvm::DbgInfoIntrinsic &);
};

#endif
