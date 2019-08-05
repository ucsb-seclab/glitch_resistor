//
// Created by Aravind Machiry on 2019-08-03.
//

#include <set>
#include "FunctionFetcherHelper.h"

// function that checks the integrity of the secure data
#define INTEREADFUNCNAME "gp_safe_read"

// helper functions for reading secure data
#define INTREADHELPERFUNCNAME "gp_read_int"
#define CHARREADHELPERFUNCNAME "gp_read_char"
#define PTRREADHELPERFUNCNAME "gp_read_ptr"

#define INTWRITEHELPERFUNCNAME "gp_write_int"
#define CHARWRITEHELPERFUNCNAME "gp_write_char"
#define PTRWRITEHELPERFUNCNAME "gp_write_ptr"

// function that write to the secure data by protecting integrity
#define INTEWRITEFUNCNAME "gp_safe_write"

using namespace GLitchPlease;

Function *FunctionFetcherHelper::getReadFunction(void) {
  if (this->readFunction == nullptr) {
    // void*, void*, void*, unsigned
    Type *parameterTypes[] = {IntegerType::getInt8PtrTy(m.getContext()),
                              IntegerType::getInt8PtrTy(m.getContext()),
                              IntegerType::getInt8PtrTy(m.getContext()),
                              IntegerType::getInt32Ty(m.getContext())};
    // int (void*, void*, void*, unsigned)
    FunctionType *readFunctionType = FunctionType::get(IntegerType::getInt32Ty(m.getContext()),
                                                       parameterTypes,false);
    // get the reference to function
    Function *func = cast<Function>(m.getOrInsertFunction(INTEREADFUNCNAME, readFunctionType));

    this->readFunction = func;
  }
  return this->readFunction;
}

Function* FunctionFetcherHelper::getReadHelperFunction(Type *targetType) {
  if(this->typeReadHelpers.find(targetType) == this->typeReadHelpers.end()) {
    // if we haven't fetched a function for this helper function?
    // void*, void*
    Type *parameterTypes[] = {IntegerType::getInt8PtrTy(m.getContext()),
                              IntegerType::getInt8PtrTy(m.getContext())};
    // targetType (void*, void*)
    FunctionType *readFunctionType = FunctionType::get(targetType,
                                                       parameterTypes,false);
    Function *func = nullptr;
    // is this a char type?
    if(targetType->isIntegerTy(8)) {
      func = cast<Function>(m.getOrInsertFunction(CHARREADHELPERFUNCNAME, readFunctionType));
    } else if(targetType->isPointerTy()) {
      // is this a pointer type?
      func = cast<Function>(m.getOrInsertFunction(PTRREADHELPERFUNCNAME, readFunctionType));
    } else if(targetType->isIntegerTy()) {
      // is this an integer type.
      func = cast<Function>(m.getOrInsertFunction(INTREADHELPERFUNCNAME, readFunctionType));
    }
    assert(func != nullptr && "Invalid helper function type requested.");

    this->typeReadHelpers[targetType] = func;
  }
  return this->typeReadHelpers[targetType];
}

Function* FunctionFetcherHelper::getWriteHelperFunction(Type *targetType) {
  if(this->typeWriteHelpers.find(targetType) == this->typeWriteHelpers.end()) {
    // if we haven't fetched a function for this helper function?
    // void*, void*, customType
    Type *parameterTypes[] = {IntegerType::getInt8PtrTy(m.getContext()),
                              IntegerType::getInt8PtrTy(m.getContext()),
                              targetType};
    // targetType void (void*, void*, customType)
    FunctionType *writeFunctionType = FunctionType::get(IntegerType::getVoidTy(m.getContext()),
                                                       parameterTypes,false);
    Function *func = nullptr;
    // is this a char type?
    if(targetType->isIntegerTy(8)) {
      func = cast<Function>(m.getOrInsertFunction(CHARWRITEHELPERFUNCNAME, writeFunctionType));
    } else if(targetType->isPointerTy()) {
      // is this a pointer type?
      func = cast<Function>(m.getOrInsertFunction(PTRWRITEHELPERFUNCNAME, writeFunctionType));
    } else if(targetType->isIntegerTy()) {
      // is this an integer type.
      func = cast<Function>(m.getOrInsertFunction(INTWRITEHELPERFUNCNAME, writeFunctionType));
    }
    assert(func != nullptr && "Invalid helper function type requested.");

    this->typeWriteHelpers[targetType] = func;
  }
  return this->typeWriteHelpers[targetType];
}

Function *FunctionFetcherHelper::getWriteFunction(void) {
  if (this->writeFunction == nullptr) {
    // void*, void*, void*, unsigned
    Type *parameterTypes[] = {IntegerType::getInt8PtrTy(m.getContext()),
                              IntegerType::getInt8PtrTy(m.getContext()),
                              IntegerType::getInt8PtrTy(m.getContext()),
                              IntegerType::getInt32Ty(m.getContext())};
    // void (void*, void*, void*, unsigned)
    FunctionType *readFunctionType = FunctionType::get(IntegerType::getVoidTy(m.getContext()),
                                                       parameterTypes,false);
    // get the reference to function
    Function *func = cast<Function>(m.getOrInsertFunction(INTEWRITEFUNCNAME, readFunctionType));

    this->writeFunction = func;
  }
  return this->writeFunction;
}

bool FunctionFetcherHelper::isFunctionSafeToModify(const Function *currF) {

  // we should not modify these functions.
  // because these are our intrumenter functions.
  std::set<std::string> instrumenterFunctions = {INTEREADFUNCNAME, INTREADHELPERFUNCNAME,
                                                 CHARREADHELPERFUNCNAME, PTRREADHELPERFUNCNAME,
                                                 INTEWRITEFUNCNAME};
  if(!currF->isDeclaration()) {
    std::string funcName = "";
    if(currF->hasName()) {
      funcName = currF->getName();
      return instrumenterFunctions.find(funcName) == instrumenterFunctions.end();
    }
    return true;
  }
  return false;
}