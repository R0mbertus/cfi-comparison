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
ControlFlowIntegrity::ReturnGraph
ControlFlowIntegrity::GetReturnGraph(Module &M, CallGraph &CG) {
    ControlFlowIntegrity::ReturnGraph RG;

    for (auto IT = scc_begin(&CG), IE = scc_end(&CG); IT != IE; ++IT) {
        // Get the current SCC and iterate over each node in the SCC
        const std::vector<CallGraphNode *> &SCC = *IT;
        for (auto *CGN : SCC) {
            // Get the caller function and iterate over each callee
            Function *Caller = CGN->getFunction();
            if (!Caller)
                continue;
            for (auto &E : *CGN) {
                // Get the callee and insert the caller into the set of callers
                Function *Callee = E.second->getFunction();
                RG[Callee].insert(Caller);
            }
        }
    }

    return RG;
}

bool ControlFlowIntegrity::runOnModule(Module &M,
                                       ControlFlowIntegrity::ReturnGraph &RG) {
    bool insertedChecks = false;

    for (Function &F : M) {
        for (inst_iterator I = inst_begin(F), E = inst_end(F); I != E; ++I) {
            if (auto *CB = dyn_cast<CallBase>(&*I)) {
                // We have a call to a function, insert check that call is to
                // valid target then either
                //   - actually go to function if valid target
                //   - panic if not valid

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
    auto &CG = AM.getResult<CallGraphAnalysis>(M);
    auto RG = ControlFlowIntegrity::GetReturnGraph(M, CG);
    bool changed = runOnModule(M, RG);

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
