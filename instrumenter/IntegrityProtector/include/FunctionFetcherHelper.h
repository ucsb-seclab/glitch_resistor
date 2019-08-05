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
   *
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
     *
     * @return
     */
    Function *getReadFunction(void);

    /***
     *
     * @param targetType
     * @return
     */
    Function* getReadHelperFunction(Type *targetType);

    /***
     *
     * @param targetType
     * @return
     */
    Function* getWriteHelperFunction(Type *targetType);

    /***
     *
     * @return
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
