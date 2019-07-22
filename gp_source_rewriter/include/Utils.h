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
extern llvm::cl::opt<bool> Verbose;

/**
 * Get the absolute path of the file name.
 * @param fileName file name to be converted.
 * @param absoluteFP absolute path of the file. (output)
 * @return true if conversation is successful else false.
 */
bool getAbsoluteFilePath(std::string fileName, std::string &absoluteFP);

/**
 * Generate numbers with maximum hamming distance.
 *
 * @param num total numbers to be generated.
 * @param output vector into which the generated numbers should be inserted.
 * @return true if successful else false
 */
bool generateNumbersWithMaximumHamming(unsigned num, std::vector<unsigned> &output);

#endif //LLVM_UTILS_H
