//========================================================================
// FILE:
//    ControlFlowIntegrity.cpp
//
// DESCRIPTION:
//    WIP
//
// USAGE:
//      $ opt -load-pass-plugin <BUILD_DIR>/lib/libControlFlowIntegrity.so `\`
//        -passes=-"cfi-base" <bitcode-file>
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

    // Create function that will check if target is valid
    auto &C = M.getContext();
    auto *VoidTy = Type::getVoidTy(C);
    auto *Int32Ty = Type::getInt32Ty(C);
    auto *Int64Ty = Type::getInt64Ty(C);
    auto *Int64PtrTy = Type::getInt64PtrTy(C);
    auto FnCallee = M.getOrInsertFunction(
        "__cfi_valid_target",
        FunctionType::get(VoidTy, {Int64PtrTy, Int64PtrTy, Int32Ty}, false));
    this->CheckFunction = cast<Function>(FnCallee.getCallee());

    // Create the function body
    auto *EntryBB = BasicBlock::Create(C, "entry", this->CheckFunction);
    auto *ForCondBB = BasicBlock::Create(C, "for.cond", this->CheckFunction);
    auto *ForBodyBB = BasicBlock::Create(C, "for.body", this->CheckFunction);
    auto *ForIncBB = BasicBlock::Create(C, "for.inc", this->CheckFunction);
    auto *ForEndBB = BasicBlock::Create(C, "for.end", this->CheckFunction);
    auto *IfBB = BasicBlock::Create(C, "for.if", this->CheckFunction);

    // Get the function arguments
    auto *Arg1 = this->CheckFunction->arg_begin();
    auto *Arg2 = std::next(this->CheckFunction->arg_begin());
    auto *Arg3 = std::next(this->CheckFunction->arg_begin(), 2);

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
    Builder.CreateRetVoid();
    // TODO actually abort
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
                for (size_t i = 0; i < ValidTargetsArray.size(); i++) {
                    auto *Idx =
                        ConstantInt::get(Type::getInt32Ty(M.getContext()), i);
                    auto *GEP = Builder.CreateGEP(ValidTargetsArrayTy,
                                                  ValidTargetsArrayGEP, Idx);
                    Builder.CreateStore(ValidTargetsArray[i], GEP);
                }

                // Insert the check function call
                std::vector<Value *> Args = {
                    CB->getCalledOperand(), ValidTargetsArrayGEP,
                    ConstantInt::get(Type::getInt32Ty(M.getContext()),
                                     ValidTargetsArray.size())};
                Builder.CreateCall(this->CheckFunction, Args);

            } else if (auto *RI = dyn_cast<ReturnInst>(&*I)) {
                // TODO: ask on how to handle backward edge CFI
                //   - ShadowStack
                //   - ReturnAddress (need to modify X86)
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
