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

int LoopNumber = 2;

namespace {
    struct LoopTilingPass : public LoopPass {
        Function *F;
        const int TileSize = 2;
        std::unordered_map<Value *, Value *> VariablesMap;
        Value *LoopCounter, *LoopBound;
        Value *NewLoopCounter;
        BasicBlock *InsertPositionHeader, *InsertPositionLatch;
        std::vector<BasicBlock *> InnermostLoopBody;
        bool IsInnermostLoop = false;

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
        }

        void findLoopCounterAndBound(Loop *L) {
            for (Instruction &I: *L->getHeader()) {
                if (isa<ICmpInst>(&I)) {
                    LoopCounter = VariablesMap[I.getOperand(0)];
                    LoopBound = VariablesMap[I.getOperand(1)];
                }
            }
        }

        void printBB(BasicBlock *BB) {
            for (auto &I: *BB) {
                I.print(errs());
                errs() << "\n";
            }
            errs() << "\n";
        }

        void createNewAllocaInstr() {
            BasicBlock &EntryBlock = F->getEntryBlock();
            BasicBlock::iterator InsertPos = EntryBlock.begin();
            while (isa<AllocaInst>(InsertPos)) {
                ++InsertPos;
            }
            IRBuilder<> Builder(&EntryBlock, InsertPos);
            AllocaInst *Alloca = Builder.CreateAlloca(Type::getInt32Ty(F->getContext()), nullptr, "");
            NewLoopCounter = Alloca;
        }

        BasicBlock *createNewLoop(BasicBlock *IP) {
            BasicBlock *IPNext = IP->getNextNode();

            BasicBlock *NewLoopPreHeader = BasicBlock::Create(F->getContext(), "", F, IPNext);
            BasicBlock *BB1 = BasicBlock::Create(F->getContext(), "", F, IPNext);
            BasicBlock *BB2 = BasicBlock::Create(F->getContext(), "", F, IPNext);
            BasicBlock *BB3 = BasicBlock::Create(F->getContext(), "", F, IPNext);
            BasicBlock *BB4 = BasicBlock::Create(F->getContext(), "", F, IPNext);

            if (auto *Branch = dyn_cast<BranchInst>(IP->getTerminator())) {
                Branch->setSuccessor(0, NewLoopPreHeader);
            }

            IRBuilder<> Builder(NewLoopPreHeader);
            Value *LoadInst = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), LoopCounter, "");
            VariablesMap[LoadInst] = LoopCounter;
            Builder.CreateStore(LoadInst, NewLoopCounter);
            Builder.CreateBr(BB1);


            Builder.SetInsertPoint(BB1);
            Value *LoadNewLoopCounter = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), NewLoopCounter, "");
            VariablesMap[LoadNewLoopCounter] = NewLoopCounter;
            Value *LoadLoopCounter = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), LoopCounter, "");
            VariablesMap[LoadLoopCounter] = LoopCounter;
            Value *Add = Builder.CreateNSWAdd(LoadLoopCounter,
                                              ConstantInt::get(Type::getInt32Ty(F->getContext()), TileSize), "");
            Value *LoadLoopBound = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), LoopBound, "");
            VariablesMap[LoadLoopBound] = LoopBound;
            Value *ICmp = Builder.CreateICmpSLT(Add, LoadLoopBound, "");
            Builder.CreateCondBr(ICmp, BB2, BB3);


            Builder.SetInsertPoint(BB2);
            LoadLoopCounter = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), LoopCounter, "");
            VariablesMap[LoadLoopCounter] = LoopCounter;
            Add = Builder.CreateNSWAdd(LoadLoopCounter, ConstantInt::get(Type::getInt32Ty(F->getContext()), TileSize),
                                       "");
            Builder.CreateBr(BB4);


            Builder.SetInsertPoint(BB3);
            LoadLoopBound = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), LoopBound, "");
            VariablesMap[LoadLoopBound] = LoopBound;
            Builder.CreateBr(BB4);


            Builder.SetInsertPoint(BB4);
            PHINode *Phi = Builder.CreatePHI(Type::getInt32Ty(F->getContext()), 2, "");
            Phi->addIncoming(Add, BB2);
            Phi->addIncoming(LoadLoopBound, BB3);
            ICmp = Builder.CreateICmpSLT(LoadNewLoopCounter, Phi, "");
            Builder.CreateCondBr(ICmp, IPNext, IPNext);


            return BB1;
        }

        void createNewLoopLatch(Loop *L, BasicBlock *IP, BasicBlock *JumpPos) {
            BasicBlock *PrevBlock = IP->getPrevNode();

            BasicBlock *NewLoopLatch = BasicBlock::Create(F->getContext(), "", F, IP);

            if (auto *Branch = dyn_cast<BranchInst>(PrevBlock->getTerminator())) {
                Branch->setSuccessor(0, NewLoopLatch);
            }

            IRBuilder<> Builder(NewLoopLatch);
            Value *LoadNewLoopCounter = Builder.CreateLoad(Type::getInt32Ty(F->getContext()), NewLoopCounter, "");
            VariablesMap[LoadNewLoopCounter] = NewLoopCounter;
            Value *Add = Builder.CreateNSWAdd(LoadNewLoopCounter,
                                              ConstantInt::get(Type::getInt32Ty(F->getContext()), 1), "");
            Builder.CreateStore(Add, NewLoopCounter);
            BranchInst *Br = Builder.CreateBr(JumpPos);
            MDNode *LoopMetadata = MDNode::get(F->getContext(), MDString::get(F->getContext(), "llvm.loop"));
            Br->setMetadata("llvm.loop", LoopMetadata);

        }

        BasicBlock *createNewLoopExitBlock(BasicBlock *IP) {
            BasicBlock *NewLoopExitBlock = BasicBlock::Create(F->getContext(), "", F, IP);
            IRBuilder<> Builder(NewLoopExitBlock);
            Builder.CreateBr(IP);


            return NewLoopExitBlock;
        }

        void addEndLoopJump(BasicBlock *Header, BasicBlock *ExitBlock) {
            BasicBlock *BB = Header;
            while (BB) {
                for (Instruction &I: *BB) {
                    if (isa<PHINode>(&I)) {
                        if (auto *Branch = dyn_cast<BranchInst>(BB->getTerminator())) {
                            Branch->setSuccessor(1, ExitBlock);
                        }
                        return;
                    }
                }
                BB = BB->getNextNode();
            }
        }

        void updateIncrement(Loop *L) {
            Value *Constant2 = ConstantInt::get(Type::getInt32Ty(F->getContext()), TileSize);
            for (auto &I: *L->getLoopLatch()) {
                if (auto *AddInst = dyn_cast<BinaryOperator>(&I)) {
                    if (AddInst->getOpcode() == Instruction::Add) {
                        AddInst->setOperand(1, Constant2);
                    }
                }
            }
        }

        void replaceOldLoopCounter() {
            for (auto &BB: InnermostLoopBody) {
                for (Instruction &I: *BB) {
                    if (LoadInst * LI = dyn_cast<LoadInst>(&I)) {
                        Value *Ptr = LI->getPointerOperand();
                        if (Ptr == LoopCounter) {
                            LI->setOperand(0, NewLoopCounter);
                        }
                    }
                }
            }
        }

        bool runOnLoop(Loop *L, LPPassManager &LPM) override {
            errs() << "\nLoop " << LoopNumber-- << "\n\n";
            F = L->getHeader()->getParent();
            mapVariables(L);
            findLoopCounterAndBound(L);
            if (!IsInnermostLoop) {
                InsertPositionHeader = L->getHeader();
                InsertPositionLatch = L->getLoopLatch();
                InnermostLoopBody = L->getBlocksVector();
                InnermostLoopBody.erase(InnermostLoopBody.begin());
                InnermostLoopBody.erase(InnermostLoopBody.end() - 1);
                IsInnermostLoop = true;
            }

            createNewAllocaInstr();
            BasicBlock *NewHeader = createNewLoop(InsertPositionHeader);
            createNewLoopLatch(L, InsertPositionLatch, NewHeader);
            BasicBlock *NewExitBlock = createNewLoopExitBlock(InsertPositionLatch);

            addEndLoopJump(NewHeader, NewExitBlock);
            updateIncrement(L);
            replaceOldLoopCounter();

            errs() << "Successfully performed Loop Tiling optimization!\n";
            return true;
        }
    };
}

char LoopTilingPass::ID = 0;
static RegisterPass <LoopTilingPass> X("loop-tiling", "Loop Tiling Pass", false, false);
