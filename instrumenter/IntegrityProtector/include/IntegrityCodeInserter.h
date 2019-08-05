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
   *
   */
  class IntegrityCodeInserter {
    IntegrityCodeInserter(Module &M, FunctionFetcherHelper &ff): m(M), fetchHelper(ff) { }

    /***
     *
     * @param src
     * @param srcInt
     * @return
     */
    bool protectData(Value *src, Value *srcInt);


  private:
    Module &m;
    FunctionFetcherHelper &fetchHelper;

    /***
     *
     * @param srcInstr
     * @param srcIntInstr
     * @return
     */
    bool replicateAndIntegrityProtect(Value *srcInstr, Value *srcIntInstr);
  };
}

#endif //INSTRUMENTER_INTEGRITYCODEINSERTER_H
