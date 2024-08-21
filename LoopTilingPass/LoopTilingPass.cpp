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

int br = 0;

namespace {
    struct LoopTilingPass : public LoopPass {
        Function *F;
        const int TileSize = 2;
        std::unordered_map<Value *, Value *> VariablesMap;
        Value *LoopCounter, *LoopBound;
        int BoundValue;
        Value *NewLoopCounter;

        static char ID; // Pass identification, replacement for typeid
        LoopTilingPass() : LoopPass(ID) {}

        void mapVariables(Loop *L) {
            Function *F = L->getHeader()->getParent();
            for (BasicBlock &BB: *F) {
                for (Instruction &I: BB) {
                    if (isa<LoadInst>(&I)) {
                        VariablesMap[&I] = I.getOperand(0);
                    }
                }
            }
//            for (auto &Var: VariablesMap) {
//                Var.first->printAsOperand(errs(), false);
//                errs() << " -> ";
//                Var.second->printAsOperand(errs(), false);
//                errs() << "\n";
//            }
        }

        void findLoopCounterAndBound(Loop *L) {
            for (Instruction &I: *L->getHeader()) {
                if (isa<ICmpInst>(&I)) {
                    LoopCounter = VariablesMap[I.getOperand(0)];
                    LoopBound = VariablesMap[I.getOperand(1)];
                    if (ConstantInt * ConstInt = dyn_cast<ConstantInt>(LoopBound)) {
                        BoundValue = ConstInt->getSExtValue();
                    }
                }
            }
//            LoopCounter->printAsOperand(errs(), false);
//            errs() << "\n";
//            LoopBound->printAsOperand(errs(), false);
//            errs() << " " << BoundValue << "\n";
        }

        void printBB(BasicBlock *BB) {
            for (auto &I: *BB) {
                I.print(errs());
                errs() << "\n";
            }
            errs() << "\n";
        }

        void createNewAllocaInstr() {
            // new variable for second loop counter
            BasicBlock &EntryBlock = F->getEntryBlock();
            BasicBlock::iterator InsertPos = EntryBlock.begin();
            while (isa<AllocaInst>(InsertPos)) {
                ++InsertPos;
            }
            IRBuilder<> Builder(&EntryBlock, InsertPos);
            AllocaInst *Alloca = Builder.CreateAlloca(Type::getInt32Ty(F->getContext()), nullptr, "");
            NewLoopCounter = Alloca;
            errs() << *Alloca << "\n\n";
        }

        void createNewLoopPreHeader(Loop *L) {
            BasicBlock *before = L->getBlocksVector()[1]; // TODO get real position
            BasicBlock *after = L->getBlocksVector()[2]; // TODO get real jump BB

            BasicBlock *NewLoopPreHeader = BasicBlock::Create(F->getContext(), "", F, before);
            IRBuilder<> Builder(NewLoopPreHeader);
            Value *LoadInst = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), LoopCounter, "");
            VariablesMap[LoadInst] = LoopCounter;
            Builder.CreateStore(LoadInst, NewLoopCounter);
            Builder.CreateBr(after);

            printBB(NewLoopPreHeader);
        }

        void createNewLoopHeader(Loop *L) {
            BasicBlock *before = L->getBlocksVector()[1]; // TODO get real position
            BasicBlock *after = L->getBlocksVector()[2]; // TODO get real jump BB

            BasicBlock *BB1 = BasicBlock::Create(F->getContext(), "", F, before);
            BasicBlock *BB2 = BasicBlock::Create(F->getContext(), "", F, before);
            BasicBlock *BB3 = BasicBlock::Create(F->getContext(), "", F, before);
            BasicBlock *BB4 = BasicBlock::Create(F->getContext(), "", F, before);

            IRBuilder<> Builder(BB1);
            // %25 = load i32, ptr %11, align 4     (load i)
            Value *LoadNewLoopCounter = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), NewLoopCounter, "");
            VariablesMap[LoadNewLoopCounter] = NewLoopCounter;
            // %26 = load i32, ptr %9, align 4      (load ii)
            Value *LoadLoopCounter = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), LoopCounter, "");
            VariablesMap[LoadLoopCounter] = LoopCounter;
            // %27 = add nsw i32 %26, 2             (ii + TileSize)
            Value *Add = Builder.CreateNSWAdd(LoadLoopCounter, ConstantInt::get(Type::getInt32Ty(F->getContext()), TileSize), "");
            // %28 = load i32, ptr %2, align 4      (load n)
            Value *LoadLoopBound = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), LoopBound, "");
            VariablesMap[LoadLoopBound] = LoopBound;
            // %29 = icmp slt i32 %27, %28          (ii + TileSize < n)
            Value *ICmp = Builder.CreateICmpSLT(Add, LoadLoopBound, "");
            // br i1 %29, label %30, label %33
            Builder.CreateCondBr(ICmp, BB2, BB3);

            printBB(BB1);

            Builder.SetInsertPoint(BB2);
            LoadLoopCounter = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), LoopCounter, "");
            VariablesMap[LoadLoopCounter] = LoopCounter;
            Add = Builder.CreateNSWAdd(LoadLoopCounter, ConstantInt::get(Type::getInt32Ty(F->getContext()), TileSize), "");
            Builder.CreateBr(BB4);

            printBB(BB2);

            Builder.SetInsertPoint(BB3);
            LoadLoopBound = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), LoopBound, "");
            VariablesMap[LoadLoopBound] = LoopBound;
            Builder.CreateBr(BB4);

            printBB(BB3);

            Builder.SetInsertPoint(BB4);
            PHINode *Phi = Builder.CreatePHI(Type::getInt32Ty(F->getContext()), 2, "");
            Phi->addIncoming(Add, BB2);
            Phi->addIncoming(LoadLoopBound, BB3);
            ICmp = Builder.CreateICmpSLT(LoadNewLoopCounter, Phi, "");
            Builder.CreateCondBr(ICmp, after, after);

            printBB(BB4);
        }

        void createNewLoopLatch(Loop *L) {
            BasicBlock *before = L->getBlocksVector()[1]; // TODO get real position
            BasicBlock *after = L->getBlocksVector()[2]; // TODO get real jump BB

            BasicBlock *NewLoopLatch = BasicBlock::Create(F->getContext(), "", F, before);
            IRBuilder<> Builder(NewLoopLatch);
            Value *LoadNewLoopCounter = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), NewLoopCounter, "");
            VariablesMap[LoadNewLoopCounter] = NewLoopCounter;
            Value *Add = Builder.CreateNSWAdd(LoadNewLoopCounter, ConstantInt::get(Type::getInt32Ty(F->getContext()), 1), "");
            Builder.CreateStore(Add, NewLoopCounter);
            BranchInst *Br = Builder.CreateBr(after);
            MDNode *LoopMetadata = MDNode::get(F->getContext(), MDString::get(F->getContext(), "llvm.loop"));
            Br->setMetadata("llvm.loop", LoopMetadata);

            printBB(NewLoopLatch);
        }

        void createNewLoopExitBlock(Loop *L) {
            BasicBlock *before = L->getBlocksVector()[1]; // TODO get real position
            BasicBlock *after = L->getBlocksVector()[2]; // TODO get real jump BB

            BasicBlock *NewLoopExitBlock = BasicBlock::Create(F->getContext(), "", F, before);
            IRBuilder<> Builder(NewLoopExitBlock);
            Builder.CreateBr(after);

            printBB(NewLoopExitBlock);
        }

        bool runOnLoop(Loop *L, LPPassManager &LPM) override {
            errs() << "\nLoop " << ++br << "\n\n";
            F = L->getHeader()->getParent();
            mapVariables(L);
            findLoopCounterAndBound(L);
            
            createNewAllocaInstr();
            createNewLoopPreHeader(L);
            createNewLoopHeader(L);
            createNewLoopLatch(L);
            createNewLoopExitBlock(L);

            return true;
        }
    };
}

char LoopTilingPass::ID = 0;
static RegisterPass <LoopTilingPass> X("loop-tiling", "Loop Tiling Pass", false, false);
