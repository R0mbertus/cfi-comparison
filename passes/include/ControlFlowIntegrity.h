//==============================================================================
// FILE:
//    ControlFlowIntegrity.h
//
// DESCRIPTION:
//    WIP
//
// License: MIT
//==============================================================================
#ifndef LLVM_CFI_INSTRUMENT_BASIC_H
#define LLVM_CFI_INSTRUMENT_BASIC_H

#include "llvm/Analysis/CallGraph.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
struct ControlFlowIntegrity : public llvm::PassInfoMixin<ControlFlowIntegrity> {
    llvm::PreservedAnalyses run(llvm::Module &M,
                                llvm::ModuleAnalysisManager &AM);
    bool runOnModule(llvm::Module &M, llvm::CallGraphAnalysis::Result &CG);

    // Without isRequired returning true, this pass will be skipped for
    // functions decorated with the optnone LLVM attribute. Note that clang -O0
    // decorates all functions with optnone.
    static bool isRequired() { return true; }
};

#endif
