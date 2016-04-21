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
using llvm::Metadata;
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
using std::unordered_set;

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
    if(DICompileUnit *comp = dyn_cast<DICompileUnit>(nod)) // usually only DICompileUnit
      for(DIGlobalVariable *var : comp->getGlobalVariables()) {
        Value *key = var->getVariable();
        assert(key);

        assert(!symbs.count(key));
        remember(*key, *var);
      }

  return false;
}

bool TraceVariables::runOnFunction(Function &fun) {
  for(BasicBlock &block : fun.getBasicBlockList())
    for(Instruction &inst : block.getInstList())
      if(DbgInfoIntrinsic *annot = dyn_cast<DbgInfoIntrinsic>(&inst)) {
        Value *key = valOf(*annot);
        if(!key || isa<Constant>(key))
          continue;

        assert(!symbs.count(key));
        remember(*key, *varOf(*annot));
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
            remember(*phi, var);
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

const unordered_set<llvm::Value *> *TraceVariables::operator[](DIVariable &reverse) const {
  return annts.count(&reverse) ? &annts.at(&reverse) : nullptr;
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

TraceVariables::size_type TraceVariables::uniq() const {
  return annts.size();
}

Value *TraceVariables::valOf(DbgInfoIntrinsic &annot) {
  Metadata *unused = nullptr;
  if(DbgDeclareInst *decl = dyn_cast<DbgDeclareInst>(&annot)) {
    if(Value *res = decl->getAddress())
      return res;
    unused = decl->getRawVariable();
  } else if(DbgValueInst *val = dyn_cast<DbgValueInst>(&annot)) {
    if(Value *res = val->getValue())
      return res;
    unused = val->getRawVariable();
  }

  if(unused)
    errs() << "Encountered unused variable: " << *unused << '\n';
  else
    errs() << "Neither a declare nor a value inst: " << annot << '\n';
  return nullptr;
}

DILocalVariable *TraceVariables::varOf(DbgInfoIntrinsic &annot) {
  if(DbgDeclareInst *decl = dyn_cast<DbgDeclareInst>(&annot))
    return decl->getVariable();
  if(DbgValueInst *val = dyn_cast<DbgValueInst>(&annot))
    return val->getVariable();
  return nullptr;
}

void TraceVariables::remember(Value &val, DIVariable &intr) {
  symbs.emplace(&val, intr);

  if(annts.count(&intr))
    annts[&intr].emplace(&val);
  else {
    unordered_set<Value *> vals = {&val};
    annts.emplace(&intr, move(vals));
  }
}

static RegisterPass<TraceVariables> X("tracevars", "Trace Variables", true, true);
