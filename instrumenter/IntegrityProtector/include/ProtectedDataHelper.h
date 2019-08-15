//
// Created by Aravind Machiry on 2019-08-04.
//

#ifndef INSTRUMENTER_PROTECTEDDATAHELPER_H
#define INSTRUMENTER_PROTECTEDDATAHELPER_H

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Instructions.h>
#include <set>

using namespace llvm;

namespace GLitchPlease {
  extern cl::opt<bool> Verbose;

  class ProtectedDataHandler {
  public:
    ProtectedDataHandler(Module &M, std::set<std::string> &protGlob) : m(M), toProtectGlobVarNames(protGlob) {
      this->toProtectDataVars.clear();
      this->shadowDataVars.clear();
    }
    /***
     *
     * @return
     */
    std::set<Value*>& getToProtectDataElements();

    /***
     *
     * @return
     */
    std::map<Value*, Value*>& getShadowDataMap();
  private:
    /***
     *
     * @param toCheckValue
     * @return
     */
    bool isAddressTaken(Value *toCheckValue);
    Module &m;
    // names of the global variables that need to be protected.
    std::set<std::string> &toProtectGlobVarNames;
    // set of the global variables that needs to be protected.
    std::set<Value*> toProtectDataVars;
    // map containing the global variables and corresponding shadow variables
    // that needs to be protected.
    std::map<Value*, Value*> shadowDataVars;

  };
}


#endif //INSTRUMENTER_PROTECTEDDATAHELPER_H
