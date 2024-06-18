//========================================================================
// FILE:
//    ControlFlowIntegrity.cpp
//
// DESCRIPTION:
//    Implementation of a simple Control Flow Integrity (CFI) pass that
//    utilizes type analysis for equivalence classes.
//
// USAGE:
//    $ clang++-15 -O0 -fpass-plugin=build/lib/libControlFlowIntegrity.so \
//          test/<file-to-build> -o test/<output-file>
//
// License: MIT
//========================================================================
#include "ControlFlowIntegrity.h"

#include "llvm/ADT/SCCIterator.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

#include <map>

using namespace llvm;

#define DEBUG_TYPE "cfi-base"

//-----------------------------------------------------------------------------
// ControlFlowIntegrity implementation
//-----------------------------------------------------------------------------
void ControlFlowIntegrity::setupTypeAnalysis(Module &M,
                                             ModuleAnalysisManager &AM) {
    // Map types to functions
    for (auto &F : M)
        this->TypeToFuncs[F.getFunctionType()].insert(&F);
}

void insertCheck(Module &M, Function *currentFunc, std::vector<Value *> Args) {
    // Create function that will check if target is valid
    auto &C = M.getContext();
    auto *VoidTy = Type::getVoidTy(C);
    auto *Int32Ty = Type::getInt32Ty(C);
    auto *Int64Ty = Type::getInt64Ty(C);
    auto *Int64PtrTy = Type::getInt64PtrTy(C);

    // Create the function body
    auto *EntryBB = BasicBlock::Create(C, "entry", currentFunc);
    auto *ForCondBB = BasicBlock::Create(C, "for.cond", currentFunc);
    auto *ForBodyBB = BasicBlock::Create(C, "for.body", currentFunc);
    auto *ForIncBB = BasicBlock::Create(C, "for.inc", currentFunc);
    auto *ForEndBB = BasicBlock::Create(C, "for.end", currentFunc);
    auto *IfBB = BasicBlock::Create(C, "for.if", currentFunc);

    // Get the function arguments
    auto *Arg1 = Args.at(0);
    auto *Arg2 = Args.at(1);
    auto *Arg3 = Args.at(2);

    // Entry block - convert target pointer to int, do pre for loop
    IRBuilder<> Builder(EntryBB);
    auto *Target = Builder.CreatePtrToInt(Arg1, Int64Ty);
    auto *i = Builder.CreateAlloca(Int32Ty, nullptr, "i");
    Builder.CreateStore(ConstantInt::get(Int32Ty, 0), i);
    Builder.CreateBr(ForCondBB);

    // Loop condition block - check if i < Arg3
    Builder.SetInsertPoint(ForCondBB);
    auto *iValue = Builder.CreateLoad(Int32Ty, i);
    auto *Cond = Builder.CreateICmpULT(iValue, Arg3);
    Builder.CreateCondBr(Cond, ForBodyBB, ForEndBB);

    // Loop body block - check if target is valid
    Builder.SetInsertPoint(ForBodyBB);
    auto *iLoad = Builder.CreateLoad(Int32Ty, i);
    auto *ValidGep = Builder.CreateGEP(Int64Ty, Arg2, iLoad);
    Value *ValidLoad = Builder.CreateLoad(Int64Ty, ValidGep);
    auto *ValidCompare = Builder.CreateICmpEQ(ValidLoad, Target);
    Builder.CreateCondBr(ValidCompare, IfBB, ForIncBB);

    // Loop if block - if target is valid, return
    Builder.SetInsertPoint(IfBB);
    Builder.CreateRetVoid();

    // Loop inc block - increment i
    Builder.SetInsertPoint(ForIncBB);
    auto *Inc = Builder.CreateAdd(iValue, ConstantInt::get(Int32Ty, 1));
    Builder.CreateStore(Inc, i);
    Builder.CreateBr(ForCondBB);

    // Loop end block - if we reacher here none were valid targets, abort
    Builder.SetInsertPoint(ForEndBB);
    auto *AbortFunc =
        Intrinsic::getDeclaration(currentFunc->getParent(), Intrinsic::trap);
    Builder.CreateCall(AbortFunc);
    Builder.CreateUnreachable();
}

void ControlFlowIntegrity::instrumentTypes(Module &M) {
    for (Function &F : M) {
        for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
            if (auto *CB = dyn_cast<CallBase>(&*I)) {
                // We have a call to a function, insert check that call is to
                // valid target then either
                //   - actually go to function if valid target
                //   - abort if not valid

                // We only care about indirect calls
                if (!CB->isIndirectCall())
                    continue;

                // Get valid target functions, convert to array of int in ir
                IRBuilder<> Builder(&*I);
                std::set<Function *> ValidTargets =
                    this->TypeToFuncs[CB->getFunctionType()];
                std::vector<Value *> ValidTargetsArray;
                for (Function *F : ValidTargets) {
                    auto *FPtr = Builder.CreatePtrToInt(
                        F, Type::getInt64Ty(M.getContext()));
                    ValidTargetsArray.push_back(FPtr);
                }
                // convert ValidTargetsArray to array of int in ir
                auto *ValidTargetsArrayTy = ArrayType::get(
                    Type::getInt64Ty(M.getContext()), ValidTargetsArray.size());
                auto *ValidTargetsArrayGEP = Builder.CreateAlloca(
                    ValidTargetsArrayTy, nullptr, "valid_targets");
                auto *Idx = Builder.CreateAlloca(
                    Type::getInt32Ty(M.getContext()), nullptr, "i");
                Builder.CreateStore(
                    ConstantInt::get(Type::getInt32Ty(M.getContext()), 0), Idx);
                for (size_t i = 0; i < ValidTargetsArray.size(); i++) {
                    auto *IdxLoad = Builder.CreateLoad(
                        Type::getInt32Ty(M.getContext()), Idx);
                    auto *GEP = Builder.CreateGEP(
                        ValidTargetsArrayTy, ValidTargetsArrayGEP, IdxLoad);
                    Builder.CreateStore(ValidTargetsArray[i], GEP);
                    auto *Inc = Builder.CreateAdd(
                        IdxLoad,
                        ConstantInt::get(Type::getInt32Ty(M.getContext()), 1));
                    Builder.CreateStore(Inc, Idx);
                }

                // Insert the check function call
                std::vector<Value *> Args = {
                    CB->getCalledOperand(), ValidTargetsArrayGEP,
                    ConstantInt::get(Type::getInt32Ty(M.getContext()),
                                     ValidTargetsArray.size())};
                insertCheck(M, CB->getFunction(), Args);
            }
        }
    }
}

PreservedAnalyses ControlFlowIntegrity::run(llvm::Module &M,
                                            llvm::ModuleAnalysisManager &AM) {
    setupTypeAnalysis(M, AM);
    instrumentTypes(M);

    return llvm::PreservedAnalyses::all();
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getControlFlowIntegrityPluginInfo() {
    return {LLVM_PLUGIN_API_VERSION, "cfi-base", LLVM_VERSION_STRING,
            [](PassBuilder &PB) {
                PB.registerPipelineEarlySimplificationEPCallback(
                    [&](ModulePassManager &MPM, auto) {
                        MPM.addPass(ControlFlowIntegrity());
                        return true;
                    });
            }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
    return getControlFlowIntegrityPluginInfo();
}
