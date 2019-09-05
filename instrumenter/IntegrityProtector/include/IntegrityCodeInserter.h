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
     * Replace all the uses of the provided instruction with the new
     * instruction.
     *
     * @param currInstr Instruction to replace.
     * @param newInstr New value to be used.
     * @return true/false
     */
    bool replaceUsesOfInstruction(Instruction *currInstr, Value *newValue);

    /***
     *  Replicate the uses of srcInstr by inserting instructions to integrity protect
     *  its reads/writes using srcIntInstr as its integrity store.
     * @param srcInstr Instruction that needs to be replicated.
     * @param srcIntInstr Instruction that corresponds to the integrity.
     * @return true if the insertion is successful else false.
     */
    bool replicateAndIntegrityProtect(Value *srcInstr, Value *srcIntInstr);

    /***
     *  Insert a cast from srcPtr to void* at the location indicated by the builder.
     *
     * @param srcPtr Pointer that needs to be casted.
     * @param builder Builder where the instruction should be inserted.
     * @return Pointer to the inserted cast.
     */
    Value* insertVoidPtrCast(Value *srcPtr, IRBuilder<> &builder);

    /***
     * Create a new local variable of provided type in srcFunction.
     * @param srcFunction Function in which the variable needs to be created.
     * @param varType Type of the local variable.
     * @return Pointer to the newly created variable.
     */
    Value* createNewLocalVar(Function *srcFunction, Type *varType);

    /***
     *  This function protects the provided variable used as an argument to a call
     *  instruction with integrity protection.
     * @param srcInstr original variable.
     * @param srcIntInstr Integerity protect copy.
     * @param CI Call instruction that is using srcInstr
     * @return true if the protection is successful else false.
     */
    bool protectCallInstr(Value *srcInstr, Value *srcIntInstr, CallInst *CI);
  };
}

#endif //INSTRUMENTER_INTEGRITYCODEINSERTER_H
