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
bool ControlFlowIntegrity::setupTypeAnalysis(Module &M,
                                             ModuleAnalysisManager &AM) {
    // Map types to functions
    std::map<FunctionType *, std::set<Function *>> TypeToFuncs;
    for (auto &F : M) {
        TypeToFuncs[F.getFunctionType()].insert(&F);
    }

    //
}

bool ControlFlowIntegrity::instrumentTypes(Module &M) {
    bool insertedChecks = false;

    for (Function &F : M) {
        for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
            if (auto *CB = dyn_cast<CallBase>(&*I)) {
                // We have a call to a function, insert check that call is to
                // valid target then either
                //   - actually go to function if valid target
                //   - panic if not valid

                // We only care about indirect calls, direct calls can't be
                // hijacked
                if (!CB->isIndirectCall())
                    continue;

            } else if (auto *RI = dyn_cast<ReturnInst>(&*I)) {
                // TODO: ask on how to handle backward edge CFI
                //   - ShadowStack
                //   - ReturnAddress (need to modify X86)
            }
        }
    }

    return insertedChecks;
}

PreservedAnalyses ControlFlowIntegrity::run(llvm::Module &M,
                                            llvm::ModuleAnalysisManager &AM) {
    bool changed = setupTypeAnalysis(M, AM);
    changed |= instrumentTypes(M);

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
