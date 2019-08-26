//
// Created by machiry at the beginning of time.
//

#include <llvm/Pass.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Analysis/GlobalsModRef.h>
#include <set>
#include <fstream>

#include "ProtectedDataHelper.h"
#include "FunctionFetcherHelper.h"
#include "IntegrityCodeInserter.h"


using namespace llvm;

namespace GLitchPlease {


  static cl::OptionCategory GPOptions("integrityprotectorpass options");

  // cl::opt<bool> Verbose("verbose",
  //                       cl::desc("Print verbose information"),
  //                       cl::init(false),
  //                       cl::cat(GPOptions));

  cl::opt<std::string> GlobalsFile("globals",
                                   cl::desc("Path to the file containing global variable that need to be protected."),
                                   cl::init("to_protect_globals.txt"),
                                   cl::cat(GPOptions));

  bool readFileLines(std::string fileName, std::set<std::string> &allLines) {
    bool toRet = false;
    std::ifstream infile(fileName);
    std::string line;
    while (std::getline(infile, line)) {
      toRet = allLines.insert(line).second || toRet;
    }
    return toRet;
  }

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
      // read the global variables to be protected.
      std::set<std::string> toProtectVars;
      if(!readFileLines(GlobalsFile, toProtectVars)) {
        llvm::errs() << "[-] Unable to read globals to protect from the file:" << GlobalsFile << "\n";
        llvm::errs() << "[-] Cannot perform integrity protection. Quitting.\n";
        return edited;
      }
      ProtectedDataHandler pDH(m, toProtectVars);
      FunctionFetcherHelper ffH(m);
      IntegrityCodeInserter integrityProtect(m, ffH);

      // identify and create shadow variables for all the variables to be protected.
      for(auto &currDataPair: pDH.getShadowDataMap()) {
        edited = integrityProtect.protectData(currDataPair.first, currDataPair.second) || edited;
      }
      return edited;
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      // may be to check for non-address taken stuff for global variables.
      AU.addRequired<GlobalsAAWrapperPass>();
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