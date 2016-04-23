#ifndef TRACE_VARIABLES_NG_H_
#define TRACE_VARIABLES_NG_H_

#include <llvm/Pass.h>
#include <unordered_map>
#include <unordered_set>

namespace llvm {
class DbgInfoIntrinsic;
class DIVariable;
class Value;
};

class TraceVariablesNg : public llvm::ModulePass {
public:
  static char ID;

  std::unordered_map<llvm::Value *, std::unordered_set<llvm::DIVariable *>> vars;
  std::unordered_map<llvm::DIVariable *, std::unordered_set<llvm::Value *>> vals;

  static std::string str(const llvm::DIVariable &, bool line_num = false);

  TraceVariablesNg();

  bool runOnModule(llvm::Module &) override;

  void print(llvm::raw_ostream &, const llvm::Module *) const override;

  void insert(llvm::Value *, llvm::DIVariable *);

private:
  // Get the register storing the variable in the program.
  static llvm::Value *valOf(llvm::DbgInfoIntrinsic *);

  // Get the source variable associated with the program variable.
  static llvm::DIVariable *varOf(llvm::DbgInfoIntrinsic *);
};

#endif
