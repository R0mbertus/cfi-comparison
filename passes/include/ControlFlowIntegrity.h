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

#include <map>

//------------------------------------------------------------------------------
// New PM interface
//------------------------------------------------------------------------------
struct ControlFlowIntegrity : public llvm::PassInfoMixin<ControlFlowIntegrity> {
    // typedef for the return graph
    typedef std::map<llvm::Function *, std::set<llvm::Function *>> ReturnGraph;

    // Function to convert the call graph to a return graph
    ReturnGraph GetReturnGraph(llvm::Module &M, llvm::CallGraph &CG);

    llvm::PreservedAnalyses run(llvm::Module &M,
                                llvm::ModuleAnalysisManager &AM);
    bool runOnModule(llvm::Module &M, ReturnGraph &RG);

    // Without isRequired returning true, this pass will be skipped for
    // functions decorated with the optnone LLVM attribute. Note that clang -O0
    // decorates all functions with optnone.
    static bool isRequired() { return true; }
};

#endif
