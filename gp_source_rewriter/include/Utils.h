//
// Created by machiry on 7/20/19.
//

#ifndef LLVM_UTILS_H
#define LLVM_UTILS_H

#include <set>
#include <ctime>
#include <string>
#include "llvm/Support/CommandLine.h"

extern llvm::cl::opt<bool> AllInitializers;


bool getAbsoluteFilePath(std::string fileName, std::string &absoluteFP);

#endif //LLVM_UTILS_H
