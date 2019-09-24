//
// Created by Aravind Machiry on 9/23/19.
//

#ifndef _BRANCHPROTECTORUTIL_H
#define _BRANCHPROTECTORUTIL_H

#include <iostream>
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <set>

using namespace llvm;
void setVerbose(bool v);

void setTag(std::string tagN);

bool insertBranch2(BranchInst &targetInstr, unsigned successorNum, Function *gdFunction);

#endif //_BRANCHPROTECTORUTIL_H
