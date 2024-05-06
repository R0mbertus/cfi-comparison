//==============================================================================
// FILE:
//    ControlFlowIntegrity.h
//
// DESCRIPTION:
//    Header for ControlFlowIntegrity.cpp. See that file for implementation
//    details.
//
// License: MIT
//==============================================================================
#ifndef LLVM_CFI_INSTRUMENT_BASIC_H
#define LLVM_CFI_INSTRUMENT_BASIC_H

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"

#include <map>

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
struct ControlFlowIntegrity : public llvm::PassInfoMixin<ControlFlowIntegrity> {
    std::map<llvm::FunctionType *, std::set<llvm::Function *>> TypeToFuncs;
    llvm::Function *CheckFunction = nullptr;

    void setupTypeAnalysis(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
    void instrumentTypes(llvm::Module &M);

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
