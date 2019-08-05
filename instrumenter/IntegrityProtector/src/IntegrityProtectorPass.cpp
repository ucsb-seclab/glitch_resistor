//
// Created by machiry at the beginning of time.
//

#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <iostream>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Support/Debug.h>
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <set>


using namespace llvm;

namespace GLitchPlease {


  static cl::OptionCategory GPOptions("integrityprotectorpass options");

  cl::opt<bool> Verbose("verbose",
                        cl::desc("Print verbose information"),
                        cl::init(false),
                        cl::cat(GPOptions));

  /***
   * The main pass.
   */
  struct IntegrityProtectorPass : public ModulePass {
  public:
    static char ID;

    IntegrityProtectorPass() : ModulePass(ID) {
    }


    ~IntegrityProtectorPass() {
    }

    bool runOnModule(Module &m) override {
      bool edited = false;
      // identify all the variables to be protected.

      // create shadow variables for each of them.

      // instrument each of the access of the secret variables.

      return edited;
    }

  };

  char IntegrityProtectorPass::ID = 0;
  // pass arg, pass desc, cfg_only, analysis only
  static RegisterPass<IntegrityProtectorPass> x("gpIntegrity",
                                            "Instrument the provided module by "
                                            "inserting necessary function calls to protect integrity "
                                            "the selected data items.",
                                            false,
                                            false);
}