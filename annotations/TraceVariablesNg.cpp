#include "TraceVariablesNg.h"

#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;
using std::string;

char TraceVariablesNg::ID = 0;

static void scopecat(raw_string_ostream &stm, DIScope &scp) {
  if(DIScopeRef nextref = scp.getScope())
    if(DIScope *next = dyn_cast<DIScope>(nextref))
      scopecat(stm, *next);
  StringRef name = scp.getName();
  if(name.size())
    stm << name << "::";
}

string TraceVariablesNg::str(const DIVariable &var, bool line_num) {
  string res;
  raw_string_ostream stm(res);
  if(DIScope *scope = var.getScope())
    scopecat(stm, *scope);
  stm << var.getName();
  if(line_num)
    if(DIFile *file = var.getFile())
      stm << "((" << file->getFilename() << ':' << var.getLine() << "))";
  stm.flush();
  return res;
}

TraceVariablesNg::TraceVariablesNg() :
    ModulePass(ID),
    vars(),
    vals() {}

bool TraceVariablesNg::runOnModule(llvm::Module &mod) {
  NamedMDNode *meta_root = mod.getNamedMetadata("llvm.dbg.cu");
  if(!meta_root) {
    errs() << "ERROR: No compilation unit metadata found; did you compile with debugging symbols?\n";
    exit(1);
  }

  // Process global variables.
  for(MDNode *node : meta_root->operands())
    if(DICompileUnit *comp_unit = dyn_cast<DICompileUnit>(node))
      for(DIGlobalVariable *val : comp_unit->getGlobalVariables()) {
        Value *key = val->getVariable();
        assert(key);
        insert(key, val);
      }

  // Process local variables.
  for(Function &fun : mod)
    for(BasicBlock &block : fun.getBasicBlockList())
      for(Instruction &inst : block.getInstList())
        if(DbgInfoIntrinsic *annot = dyn_cast<DbgInfoIntrinsic>(&inst))
          if(Value *key = valOf(annot))
            // This source variable is used in the program.
            insert(key, varOf(annot));

  return false;
}

void TraceVariablesNg::print(raw_ostream &stm, const Module *mod) const {
  for(auto mapping : vals) {
    stm << str(*mapping.first, true) << " is carried in registers:\n";
    for(Value *var : mapping.second) {
      stm << '\t';
      var->printAsOperand(stm, false);
    }
    stm << '\n';
  }
}

void TraceVariablesNg::insert(Value *val, DIVariable *var) {
  assert(val);
  assert(var);

  vars[val].emplace(var);
  vals[var].emplace(val);
}

Value *TraceVariablesNg::valOf(DbgInfoIntrinsic *inf) {
  assert(inf);

  Value *res = nullptr;
  if(DbgDeclareInst *decl = dyn_cast<DbgDeclareInst>(inf))
    res = decl->getAddress();
  else if(DbgValueInst *stor = dyn_cast<DbgValueInst>(inf))
    res = stor->getValue();
  else {
    errs() << "WARNING: Tried to valOf() unhandled DbgDeclareInst: " << inf << '\n';
    return nullptr;
  }

  if(!res)
    errs() << "INFO: Encountered unused variable: " << str(*varOf(inf), true) << '\n';
  else if(isa<Constant>(res))
    res = nullptr;
  return res;
}

DIVariable *TraceVariablesNg::varOf(DbgInfoIntrinsic *inf) {
  assert(inf);

  DIVariable *res = nullptr;
  if(DbgDeclareInst *decl = dyn_cast<DbgDeclareInst>(inf))
    res = decl->getVariable();
  else if(DbgValueInst *stor = dyn_cast<DbgValueInst>(inf))
    res = stor->getVariable();
  else {
    errs() << "WARNING: Tried to varOf() unhandled DbgDeclareInst: " << inf << '\n';
    return nullptr;
  }

  assert(res);
  return res;
}

static RegisterPass<TraceVariablesNg> tracevarsng("tracevarsng", "Trace Variables: The Next Generation", true, true);
