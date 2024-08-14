#include "llvm/IR/Instruction.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/LoopPeel.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/Transforms/Utils/SizeOpts.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopUnrollAnalyzer.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/LoopUtils.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

namespace {
    struct LoopTilingPass : public LoopPass {
        static char ID; // Pass identification, replacement for typeid
        LoopTilingPass() : LoopPass(ID) {}

        bool runOnLoop(Loop *L, LPPassManager &LPM) override {
            errs() << "Loop pass test\n";
            return false;
        }
    };
}

char LoopTilingPass::ID = 0;
static RegisterPass<LoopTilingPass> X("loop-tiling", "Loop Tiling Pass", false, false);
