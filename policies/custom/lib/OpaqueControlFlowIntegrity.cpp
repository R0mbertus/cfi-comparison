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

#include <algorithm>
#include <map>

using namespace llvm;

#define DEBUG_TYPE "cfi-opaque"

//-----------------------------------------------------------------------------
// ControlFlowIntegrity implementation
//-----------------------------------------------------------------------------
void ControlFlowIntegrity::setupTypeAnalysis(Module &M,
                                             ModuleAnalysisManager &AM) {
    // Map types to functions
    for (auto &F : M)
        this->TypeToFuncs[F.getFunctionType()].insert(&F);
}

void insertCheck(Module &M, CallBase *CB) {
    // Create function that will check if target is valid
    IRBuilder<> Builder(CB);

    auto *F = CB->getFunction();
    // Cast the callee to integer type
    auto *Callee = CB->getCalledOperand();
    auto *CalleeInt =
        Builder.CreatePtrToInt(Callee, Builder.getInt64Ty(), "callee.int");

    // Create constants for min and max addresses
    auto *MinAddr = Builder.getInt64(0);
    auto *MaxAddr = Builder.getInt64(INT64_MAX - 100);

    // Compare the callee address with the min and max addresses
    auto *CmpMin = Builder.CreateICmpUGE(CalleeInt, MinAddr, "cmp.min");
    auto *CmpMax = Builder.CreateICmpULE(CalleeInt, MaxAddr, "cmp.max");
    auto *InRange = Builder.CreateAnd(CmpMin, CmpMax, "in.range");

    // Create blocks for the check and abort
    auto *ParentFunc = CB->getFunction();
    auto *OriginalBlock = CB->getParent();
    auto *ContBlock = OriginalBlock->splitBasicBlock(CB, "cont.block");
    auto *CheckBlock = BasicBlock::Create(F->getContext(), "check.block",
                                          ParentFunc, ContBlock);

    // Move the terminator of the original block to the check block
    OriginalBlock->getTerminator()->eraseFromParent();
    Builder.SetInsertPoint(OriginalBlock);
    Builder.CreateCondBr(InRange, ContBlock, CheckBlock);

    // Set insert point to the check block
    Builder.SetInsertPoint(CheckBlock);
    auto *AbortFunc =
        Intrinsic::getDeclaration(F->getParent(), Intrinsic::trap);
    Builder.CreateCall(AbortFunc);
    Builder.CreateUnreachable();
}

void ControlFlowIntegrity::instrumentTypes(Module &M) {
    std::vector<CallBase *> Calls;

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

                Calls.push_back(CB);
            }
        }
    }

    for (CallBase *CB : Calls) {
        IRBuilder<> Builder(CB);
        std::set<Function *> ValidTargets =
            this->TypeToFuncs[CB->getFunctionType()];
        std::vector<Value *> ValidTargetsArray;
        for (Function *F : ValidTargets) {
            auto *FPtr =
                Builder.CreatePtrToInt(F, Type::getInt64Ty(M.getContext()));
            ValidTargetsArray.push_back(FPtr);
        }

        insertCheck(M, CB);
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
