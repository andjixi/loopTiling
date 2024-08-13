#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
    struct LoopTilingPass : public FunctionPass {
        static char ID; // Pass identification, replacement for typeid
        LoopTilingPass() : FunctionPass(ID) {}

        bool runOnFunction(Function &F) override {
            errs() << "pass test\n";
            return false;
        }
    };
}

char LoopTilingPass::ID = 0;
static RegisterPass<LoopTilingPass> X("loop-tiling", "Loop Tiling Pass");
