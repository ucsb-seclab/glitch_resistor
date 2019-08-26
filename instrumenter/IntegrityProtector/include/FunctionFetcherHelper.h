//
// Created by Aravind Machiry on 2019-08-03.
//

#ifndef INSTRUMENTER_FUNCTIONFETCHERHELPER_H
#define INSTRUMENTER_FUNCTIONFETCHERHELPER_H

#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>

using namespace llvm;

namespace GLitchPlease {
  /***
   * This is a helper class that identifies read and write
   * helper functions.
   */
  class FunctionFetcherHelper {
  public:

    FunctionFetcherHelper(Module &M) : m(M) {
      this->readFunction = nullptr;
      this->writeFunction = nullptr;
      this->typeReadHelpers.clear();
      this->typeWriteHelpers.clear();
    }

    /***
     *  Get the pointer to Raw read function.
     * @return Pointer to the function used to read raw value.
     */
    Function *getReadFunction(void);

    /***
     * Get the pointer to the helper function that reads the data
     * corresponding to the provided type.
     * @param targetType Type of the data to read.
     * @return Pointer to the read helper function.
     */
    Function* getReadHelperFunction(Type *targetType);

    /***
     * Similar to the read helper function, this is the write counterpart.
     * It fetches a pointer to the function that writes data corresponding
     * to the provided type.
     * @param targetType Type of the data to write.
     * @return Pointer to the write helper function.
     */
    Function* getWriteHelperFunction(Type *targetType);

    /***
     * Get the pointer to Raw write function.
     * @return Pointer to the function used to write raw value.
     */
    Function *getWriteFunction(void);

    /***
     * Check if the provided function is safe to modify.
     * @param currF Function to check.
     * @return flag that indicates if we can modify the current function or not.
     */
    bool isFunctionSafeToModify(const Function *currF);

  private:
    Function *readFunction;
    Function *writeFunction;
    std::map<Type*, Function*> typeReadHelpers;
    std::map<Type*, Function*> typeWriteHelpers;
    Module &m;
  };
}
#endif //INSTRUMENTER_FUNCTIONFETCHERHELPER_H
