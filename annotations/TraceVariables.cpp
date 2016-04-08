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
using std::advance;
using std::distance;
using std::pair;
using std::unordered_map;

const ssize_t TraceVariables::NOT_FOUND = -1;

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

        assert(!symbs.count(key));
        symbs.emplace(key, *var);
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

        assert(!symbs.count(key));
        symbs.emplace(key, *varOf(*annot));
      }

  for(BasicBlock &block : fun.getBasicBlockList())
    for(Instruction &inst : block.getInstList())
      if(PHINode *phi = dyn_cast<PHINode>(&inst))
        for(Use &use : phi->operands())
          if(symbs.count(&*use)) {
            DIVariable &var = symbs.at(&*use);
            if(symbs.count(phi)) {
              assert(&symbs.at(phi) == &var);
              continue;
            }
            symbs.emplace(phi, var);
          }

  return false;
}

ssize_t TraceVariables::index(Value &val) const {
  const_iterator it = symbs.find(&val);
  return it != symbs.end() ? distance(begin(), it) : NOT_FOUND;
}

DIVariable *TraceVariables::operator[](Value &val) const {
  return symbs.count(&val) ? &symbs.at(&val) : nullptr;
}

pair<Value *, DIVariable &> TraceVariables::operator[](TraceVariables::size_type val) const {
  const_iterator it = begin();
  advance(it, val);
  return *it;
}

TraceVariables::iterator TraceVariables::begin() {
  return symbs.begin();
}

TraceVariables::const_iterator TraceVariables::begin() const {
  return symbs.begin();
}

TraceVariables::iterator TraceVariables::end() {
  return symbs.end();
}

TraceVariables::const_iterator TraceVariables::end() const {
  return symbs.end();
}

TraceVariables::size_type TraceVariables::size() const {
  return symbs.size();
}

Value *TraceVariables::valOf(DbgInfoIntrinsic &annot) {
  if(DbgDeclareInst *decl = dyn_cast<DbgDeclareInst>(&annot))
    return decl->getAddress();
  if(DbgValueInst *val = dyn_cast<DbgValueInst>(&annot))
    return val->getValue();
  return nullptr;
}

DILocalVariable *TraceVariables::varOf(DbgInfoIntrinsic &annot) {
  if(DbgDeclareInst *decl = dyn_cast<DbgDeclareInst>(&annot))
    return decl->getVariable();
  if(DbgValueInst *val = dyn_cast<DbgValueInst>(&annot))
    return val->getVariable();
  return nullptr;
}

static RegisterPass<TraceVariables> X("tracevars", "Trace Variables", true, true);
