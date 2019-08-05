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
    ProtectedDataHandler(Module &M) : m(M) { }
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

  };

  class IntegrityCheckInserter {
  public:
    IntegrityCheckInserter(Module &M): m(M) {}

    bool protectAccess(Value *original, Value *integrity);
  private:
    Module &m;
  };
}


#endif //INSTRUMENTER_PROTECTEDDATAHELPER_H
