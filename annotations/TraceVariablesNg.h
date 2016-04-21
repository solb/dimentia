#ifndef TRACE_VARIABLES_NG_H_
#define TRACE_VARIABLES_NG_H_

#include <llvm/Pass.h>
#include <unordered_map>

namespace llvm {
class DbgInfoIntrinsic;
class DIVariable;
class Value;
};

class TraceVariablesNg : public llvm::ModulePass {
public:
  static char ID;

  std::unordered_multimap<llvm::Value *, llvm::DIVariable *> vars;
  std::unordered_multimap<llvm::DIVariable *, llvm::Value *> vals;

  static std::string str(const llvm::DIVariable &, bool line_num = false);

  TraceVariablesNg();

  bool runOnModule(llvm::Module &) override;

  void print(llvm::raw_ostream &, const llvm::Module *) const override;

  void insert(llvm::Value *, llvm::DIVariable *);
  std::vector<llvm::DIVariable *> unique_range() const;

private:
  static llvm::Value *valOf(llvm::DbgInfoIntrinsic *);
  static llvm::DIVariable *varOf(llvm::DbgInfoIntrinsic *);
};

#endif
