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
  // extern cl::opt<bool> Verbose;
  extern std::string TAG;
  /***
   *  This class handles looking up data values that need to be protected.
   *  This also creates duplicate variables, that store the integrity value
   *  for the corresponding original global variable.
   */
  class ProtectedDataHandler {
  public:
    
    ProtectedDataHandler(Module &M, std::set<std::string> &protGlob) : m(M), toProtectGlobVarNames(protGlob) {
      this->toProtectDataVars.clear();
      this->shadowDataVars.clear();
    }
    /***
     * Get the set of the values that need to be protected.
     * @return Set of values that need to be protected.
     */
    std::set<Value*>& getToProtectDataElements();

    /***
     *  Get the map of values to protect and the corresponding values that
     *  has the integrity value for the corresponding data.
     * @return Map of data values and corresponding integrity values.
     */
    std::map<Value*, Value*>& getShadowDataMap();
  private:
    /***
     * Check if the provided value has its address taken
     * i.e., the address of it is stored somewhere.
     * @param toCheckValue The value which should be checked.
     * @return true if taken else false
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
