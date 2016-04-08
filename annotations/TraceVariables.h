#ifndef TRACE_VARIABLES_H_
#define TRACE_VARIABLES_H_

#include <llvm/Pass.h>
#include <unordered_map>

namespace llvm {
class DbgInfoIntrinsic;
class DILocalVariable;
class DIVariable;
class Value;
}

class TraceVariables : public llvm::FunctionPass {
public:
  static const ssize_t NOT_FOUND;

  static char ID;

private:
  std::unordered_map<llvm::Value *, llvm::DIVariable &> symbs;

public:
  typedef std::unordered_map<llvm::Value *, llvm::DIVariable &>::size_type size_type;
  typedef ssize_t difference_type;

  typedef std::unordered_map<llvm::Value *, llvm::DIVariable &>::iterator iterator;
  typedef std::unordered_map<llvm::Value *, llvm::DIVariable &>::const_iterator const_iterator;

  TraceVariables();

  bool doInitialization(llvm::Module &) override;
  bool runOnFunction(llvm::Function &) override;

  difference_type index(llvm::Value &) const;
  llvm::DIVariable *operator[](llvm::Value &) const;
  std::pair<llvm::Value *, llvm::DIVariable &> operator[](size_type) const;

  iterator begin();
  const_iterator begin() const;
  iterator end();
  const_iterator end() const;

  size_type size() const;

private:
  static llvm::Value *valOf(llvm::DbgInfoIntrinsic &);
  static llvm::DILocalVariable *varOf(llvm::DbgInfoIntrinsic &);
};

#endif
