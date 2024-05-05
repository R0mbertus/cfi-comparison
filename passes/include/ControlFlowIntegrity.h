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

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
struct ControlFlowIntegrity : public llvm::PassInfoMixin<ControlFlowIntegrity> {
    bool setupTypeAnalysis(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
    bool instrumentTypes(llvm::Module &M);

    // -------------------------------------------------------------------------
    // LLVM required methods
    // -------------------------------------------------------------------------
    llvm::PreservedAnalyses run(llvm::Module &M,
                                llvm::ModuleAnalysisManager &AM);

    // Without isRequired returning true, this pass will be skipped for
    // functions decorated with the optnone LLVM attribute. Note that clang -O0
    // decorates all functions with optnone.
    static bool isRequired() { return true; }
};

#endif
