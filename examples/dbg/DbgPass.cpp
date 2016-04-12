#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/Analysis/CFG.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/ValueSymbolTable.h"

#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/raw_ostream.h"

#include <cassert>
#include <set>
#include <map>
#include <sstream>

#define TRACE(x) errs() << #x << " = " << x << "\n"
#define _ << " _ " <<

namespace llvm {
  struct MyDbgPass : public FunctionPass {
    static char ID;
    MyDbgPass() : FunctionPass(ID) {}

    bool runOnFunction(Function &F) override {
      for (auto inst_it = inst_begin(F); inst_it != inst_end(F); ++inst_it) {
        Instruction* inst = &*inst_it;
        outs() << *inst << "\n";
        if (const DebugLoc diloc = inst->getDebugLoc()) {
          TRACE(diloc->getLine() _ diloc->getFilename() _ diloc->getDirectory());
        }
      }

      TRACE("\n");
      BasicBlock* firstBB = &F.getBasicBlockList().front();
      Instruction* firstInst = &firstBB->getInstList().front();

      TRACE("INTRINSIC");
      for(BasicBlock &block : F.getBasicBlockList())
        for(Instruction &inst : block.getInstList())
          if(DbgInfoIntrinsic *annot = dyn_cast<DbgInfoIntrinsic>(&inst)) {
            TRACE(inst _ *annot);
          }



      return false;
    }
  };

  char MyDbgPass::ID = 0;
  static RegisterPass<MyDbgPass> X("my-dbg", "My MyDbgPass", false, false);
}


