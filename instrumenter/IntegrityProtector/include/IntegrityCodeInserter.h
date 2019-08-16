//
// Created by Aravind Machiry on 2019-08-04.
//

#ifndef INSTRUMENTER_INTEGRITYCODEINSERTER_H
#define INSTRUMENTER_INTEGRITYCODEINSERTER_H

#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Function.h>
#include "FunctionFetcherHelper.h"

using namespace llvm;

namespace GLitchPlease {
  extern cl::opt<bool> Verbose;
  /***
   *  Class that deals with adding code to integrity protect the provided data.
   */
  class IntegrityCodeInserter {
  public:
    IntegrityCodeInserter(Module &M, FunctionFetcherHelper &ff): m(M), fetchHelper(ff) { }

    /***
     *   Protect all access to the src using srcInt as the data
     *   value storing corresponding integrity value.
     * @param src Pointer to the data that needs to be protected.
     * @param srcInt Pointer to the data that contains the integrity of the data.
     * @return true if there was any code inserted.
     */
    bool protectData(Value *src, Value *srcInt);


  private:
    Module &m;
    FunctionFetcherHelper &fetchHelper;

    /***
     *  Check if the provided instruction can be replicated easily
     *  by just replacing operands.
     *
     * @param currInstr Instruction that needs to be replicated.
     * @return true/false
     */
    bool canReplicateUsingReplacement(Value *currInstr);

    /***
     *  Replicate the uses of srcInstr by inserting instructions to integrity protect
     *  its reads/writes using srcIntInstr as its integrity store.
     * @param srcInstr Instruction that needs to be replicated.
     * @param srcIntInstr Instruction that corresponds to the integrity.
     * @return true if the insertion is successful else false.
     */
    bool replicateAndIntegrityProtect(Value *srcInstr, Value *srcIntInstr);
  };
}

#endif //INSTRUMENTER_INTEGRITYCODEINSERTER_H
