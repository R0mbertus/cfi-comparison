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

#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

#define DEBUG_TYPE "cfi-base"

//-----------------------------------------------------------------------------
// ControlFlowIntegrity implementation
//-----------------------------------------------------------------------------
bool ControlFlowIntegrity::runOnModule(Module &M,
                                       CallGraphAnalysis::Result &CG) {
    bool insertedChecks = false;

    for (Function &F : M) {
        for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
            if (auto *CB = dyn_cast<CallBase>(&*I)) {
                // We have a call to a function, insert check that call is to
                // valid target (using call graph), else panic
                errs() << *CB->getCalledOperand() << "\n";
            } else if (auto *RI = dyn_cast<ReturnInst>(&*I)) {
                // We have a return, insert check that the return is to a valid
                // target (using call graph), else panic
            }
        }
    }

    return insertedChecks;
}

PreservedAnalyses ControlFlowIntegrity::run(llvm::Module &M,
                                            llvm::ModuleAnalysisManager &AM) {
    auto &CG = AM.getResult<CallGraphAnalysis>(M);
    bool changed = runOnModule(M, CG);

    return (changed ? llvm::PreservedAnalyses::none()
                    : llvm::PreservedAnalyses::all());
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
