#include "TraceVariables.h"

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>
#include <cassert>

using llvm::BasicBlock;
using llvm::Constant;
using llvm::DbgDeclareInst;
using llvm::DbgInfoIntrinsic;
using llvm::DbgValueInst;
using llvm::DICompileUnit;
using llvm::DIGlobalVariable;
using llvm::DILocalVariable;
using llvm::DIVariable;
using llvm::dyn_cast;
using llvm::errs;
using llvm::Function;
using llvm::Instruction;
using llvm::isa;
using llvm::MDNode;
using llvm::Module;
using llvm::NamedMDNode;
using llvm::PHINode;
using llvm::RegisterPass;
using llvm::Use;
using llvm::Value;
using std::unordered_map;

char TraceVariables::ID = 0;

TraceVariables::TraceVariables() :
	FunctionPass(ID) {}

bool TraceVariables::doInitialization(Module &mod) {
	NamedMDNode *meta = mod.getNamedMetadata("llvm.dbg.cu");
	if(!meta) {
		errs() << "ERROR: No compilation unit metadata found.  Did you compile with debugging symbols?\n";
		assert(false);
	}

	for(MDNode *nod : meta->operands())
		if(DICompileUnit *comp = dyn_cast<DICompileUnit>(nod))
			for(DIGlobalVariable *var : comp->getGlobalVariables()) {
				Value *key = var->getVariable();
				assert(key);

				assert(!globals.count(key));
				globals.emplace(key, *var);
			}

	return false;
}

bool TraceVariables::runOnFunction(Function &fun) {
	for(BasicBlock &block : fun.getBasicBlockList())
		for(Instruction &inst : block.getInstList())
			if(DbgInfoIntrinsic *annot = dyn_cast<DbgInfoIntrinsic>(&inst)) {
				Value *key = valOf(*annot);
				assert(key);
				if(isa<Constant>(key))
					continue;

				assert(!locals.count(key));
				locals.emplace(key, *varOf(*annot));
			}

	for(BasicBlock &block : fun.getBasicBlockList())
		for(Instruction &inst : block.getInstList())
			if(PHINode *phi = dyn_cast<PHINode>(&inst))
				for(Use &use : phi->operands())
					if(locals.count(&*use)) {
						DILocalVariable &var = locals.at(&*use);
						if(locals.count(phi)) {
							assert(&locals.at(phi) == &var);
							continue;
						}
						locals.emplace(phi, var);
					}

	return false;
}

DIVariable *TraceVariables::operator[](Value &val) {
	if(locals.count(&val))
		return &locals.at(&val);
	if(globals.count(&val))
		return &globals.at(&val);
	return NULL;
}

Value *TraceVariables::valOf(DbgInfoIntrinsic &annot) {
	if(DbgDeclareInst *decl = dyn_cast<DbgDeclareInst>(&annot))
		return decl->getAddress();
	if(DbgValueInst *val = dyn_cast<DbgValueInst>(&annot))
		return val->getValue();
	return NULL;
}

DILocalVariable *TraceVariables::varOf(DbgInfoIntrinsic &annot) {
	if(DbgDeclareInst *decl = dyn_cast<DbgDeclareInst>(&annot))
		return decl->getVariable();
	if(DbgValueInst *val = dyn_cast<DbgValueInst>(&annot))
		return val->getVariable();
	return NULL;
}

static RegisterPass<TraceVariables> X("tracevars", "Trace Variables", true, true);
