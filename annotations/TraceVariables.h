#ifndef TRACE_VARIABLES_H_
#define TRACE_VARIABLES_H_

#include <llvm/Pass.h>
#include <unordered_map>

namespace llvm {
class DbgInfoIntrinsic;
class DILocalVariable;
class Value;
}

class TraceVariables : public llvm::FunctionPass {
public:
	static char ID;

private:
	std::unordered_map<llvm::Value *, llvm::DbgInfoIntrinsic &> locals;

public:
	TraceVariables();

	bool runOnFunction(llvm::Function &) override;
	bool doFinalization(llvm::Module &) override;

private:
	static llvm::Value *valOf(llvm::DbgInfoIntrinsic &);
	static llvm::DILocalVariable *varOf(llvm::DbgInfoIntrinsic &);
};

#endif
