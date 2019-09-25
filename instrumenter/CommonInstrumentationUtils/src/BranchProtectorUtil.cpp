//
// Created by Aravind Machiry on 9/23/19.
//

#include "BranchProtectorUtil.h"

static bool Verbose = false;
static std::string TAG = "\033[1;31m[GR/BranchUtil]\033[0m ";

static bool canReplicate(Instruction *currIn);
static bool canReplicateOperands(Instruction *currIn);
static bool
recursivelyGetInstructionsToReplicate(Instruction *currInstr,
                                      std::vector<Instruction *> &allInstrs);
static bool getAllInstrToReplicate(BranchInst &targetInstr,
                                   std::vector<Instruction *> &allInstrs);
static bool duplicateInstructions(IRBuilder<> &builder,
                                  BranchInst &targetBrInst,
                                  std::vector<Instruction *> &allInstrs);

void setVerbose(bool v) { Verbose = v; }

void setTag(std::string tagN) { TAG = tagN; }

/***
 * This function checks if the provided instruction can be replicated.
 * @param currIn Instruction to be checked.
 * @return true if can be replicate else false.
 */
static bool canReplicate(Instruction *currIn) {
  // Let's not replicate volatile memory loads
  // These values SHOULD actually change between comparisions
  if (LoadInst *LI = dyn_cast<LoadInst>(currIn)) {
    if (LI->isVolatile()) {
      return false;
    }
  }

  // it should not be a call or a load or constant instruction.
  // PHINodes also pose issues since they can span multiple basic blocks
  return !(dyn_cast<CallInst>(currIn) || dyn_cast<Constant>(currIn) ||
           dyn_cast<PHINode>(currIn));
}

/***
 * This function checked if the operands ofb the provided instruction
 * can be replicated.
 * @param currIn Instruction to check.
 * @return true if the operands can be replicated.
 */
static bool canReplicateOperands(Instruction *currIn) {
  // it should not be a call or a load or constant instruction.
  return !(dyn_cast<CallInst>(currIn) || dyn_cast<LoadInst>(currIn) ||
           dyn_cast<Constant>(currIn));
}

/***
 * This function checks if the provided instruction can be replicated as a
 * value. This used to negate operands in our branch
 *
 * @param currIn Instruction to check.
 * @return true if the operands can be replicated.
 */
bool canReplicateValue(Instruction *currIn) {
  // it should not be a call or a load or constant instruction.
  // TODO: Figure out exactly what we should be checking for here.
  return !(dyn_cast<CallInst>(currIn) || dyn_cast<LoadInst>(currIn));
}

/***
 *  This function gets all the non-memory instructions that needed to
 * replicate to replicate the provided instruction.
 * @param currInstr Instruction that needed to replicate.
 * @param allInstrs list of instructions that needed to replicate.
 * @return true if there are any instructions that needed to replicate.
 */
static bool
recursivelyGetInstructionsToReplicate(Instruction *currInstr,
                                      std::vector<Instruction *> &allInstrs) {
  bool hasInstrInserted = false;
  if (currInstr != nullptr) {
    // check if the current instruction is already visited?
    if (std::find(allInstrs.begin(), allInstrs.end(), currInstr) ==
        allInstrs.end()) {

      if (!canReplicate(currInstr)) {
        if (Verbose)
          errs() << TAG << "Skipping " << *currInstr << "\n";
        return hasInstrInserted;
      }
      allInstrs.insert(allInstrs.begin(), currInstr);
      hasInstrInserted = true;
      if (canReplicateOperands(currInstr)) {
        assert(dyn_cast<StoreInst>(currInstr) == nullptr &&
               "We cannot have store instruction as an operand.");
        for (unsigned i = 0; i < currInstr->getNumOperands(); i++) {
          Value *currOp = currInstr->getOperand(i);
          if (Instruction *CI = dyn_cast<Instruction>(currOp)) {
            if (canReplicate(CI)) {
              hasInstrInserted =
                  recursivelyGetInstructionsToReplicate(CI, allInstrs) ||
                  hasInstrInserted;
            } else if (Verbose) {
              errs() << TAG << "Skipping " << *CI << "\n";
            }
          }
        }
      }
    }
  }
  return hasInstrInserted;
}

/***
 * For a given branch instruction to be replicated,
 * this function gets all the instructions that need to be
 * replicated to faithfully replicate the provided branch instruction.
 *
 * @param targetInstr Target branch instruction to replicate.
 * @param allInstrs List of all instructions that need to be replicated.
 * @return true if one or more instructions need to be replicated.
 */
static bool getAllInstrToReplicate(BranchInst &targetInstr,
                                   std::vector<Instruction *> &allInstrs) {
  assert(targetInstr.isConditional() && "This has to be conditional.");
  if (Instruction *CI = dyn_cast<Instruction>(targetInstr.getCondition())) {
    return recursivelyGetInstructionsToReplicate(CI, allInstrs);
  }
  return false;
}

/***
 * Duplicate all the instructions at the provided insertion point.
 * @param builder point at which the instruction needs to be inserted.
 * @param targetBrInst Branch instruction because of which the replication
 * should be done.
 * @param allInstrs vector of all the instructions that needed to be
 * replicated.
 * @return true if the insertion is successful.
 */
static bool duplicateInstructions(IRBuilder<> &builder,
                                  BranchInst &targetBrInst,
                                  std::vector<Instruction *> &allInstrs) {
  std::map<Instruction *, Instruction *> replicatedInstrs;
  for (auto currIn : allInstrs) {
    Instruction *newInstr = currIn->clone();
    if (ICmpInst *currICMPInstr = dyn_cast<ICmpInst>(currIn)) {
      CmpInst::Predicate currInstrP = currICMPInstr->getPredicate();
      if (currInstrP == CmpInst::ICMP_EQ || currInstrP == CmpInst::ICMP_NE) {
        assert(currICMPInstr->getNumOperands() == 2 &&
               "Expect == and != to have 2 operands.");
        // first get the operands.
        Value *op1 = currICMPInstr->getOperand(0);
        Value *op2 = currICMPInstr->getOperand(1);

        // Make sure that we won't cause any issues when we replicate these
        // values
        bool replicatable = false;

        // see if these are already replicated? if yes, get the replicated
        // copies.
        if (Verbose)
          op1->dump();
        if (Instruction *opInstr = dyn_cast<Instruction>(op1)) {
          if (Verbose)
            errs() << "Replicatable " << canReplicateValue(opInstr) << "\n";
          replicatable |= canReplicateValue(opInstr);
          if (replicatedInstrs.find(opInstr) != replicatedInstrs.end()) {
            op1 = replicatedInstrs[opInstr];
          }
        }

        if (Verbose)
          op2->dump();
        if (Instruction *opInstr = dyn_cast<Instruction>(op2)) {
          if (Verbose)
            errs() << "Replicatable " << canReplicateValue(opInstr) << "\n";
          replicatable |= canReplicateValue(opInstr);
          if (replicatedInstrs.find(opInstr) != replicatedInstrs.end()) {
            op2 = replicatedInstrs[opInstr];
          }
        }

        if (replicatable) {
          errs() << TAG << "Negated.\n";
          // Now, negate them. (When possible)
          Value *xorOp1 =
              builder.CreateBinOp(Instruction::BinaryOps::Xor, op1,
                                  ConstantInt::get(op1->getType(), ~0));
          Value *xorOp2 =
              builder.CreateBinOp(Instruction::BinaryOps::Xor, op2,
                                  ConstantInt::get(op2->getType(), ~0));

          // replace the operands with Xored operands.
          newInstr->replaceUsesOfWith(currICMPInstr->getOperand(0), xorOp1);
          newInstr->replaceUsesOfWith(currICMPInstr->getOperand(1), xorOp2);
        }
      }
    }
    if (LoadInst *LI = dyn_cast<LoadInst>(currIn)) {
      // if this is a load instruction? make it volatile
      LI->setVolatile(true);
      // also make the new instruction volatile.
      (dyn_cast<LoadInst>(newInstr))->setVolatile(true);
      // now insert the newly created load right then and there
      // since, we marked it as volatile, it will not be optimized.
      auto instrIterator = LI->getIterator();
      instrIterator++;
      IRBuilder<> newBuilder(LI->getParent());
      newBuilder.SetInsertPoint(&(*instrIterator));
      newBuilder.Insert(newInstr);
    } else {
      builder.Insert(newInstr);
    }

    assert(newInstr != nullptr &&
           "New instruction to insert cannot be nullptr");
    replicatedInstrs[currIn] = newInstr;
    for (unsigned i = 0; i < newInstr->getNumOperands(); i++) {
      Value *currOp = newInstr->getOperand(i);
      if (Instruction *opInstr = dyn_cast<Instruction>(currOp)) {
        if (replicatedInstrs.find(opInstr) != replicatedInstrs.end()) {
          newInstr->replaceUsesOfWith(currOp, replicatedInstrs[opInstr]);
        }
      }
    }
  }
  // Okay, now that we replicated all the instructions.
  // change the condition of the branch instruction to refer to the newly
  // inserted instruction.
  if (Instruction *CI = dyn_cast<Instruction>(targetBrInst.getCondition())) {
    if (replicatedInstrs.find(CI) != replicatedInstrs.end()) {
      targetBrInst.replaceUsesOfWith(targetBrInst.getCondition(),
                                     replicatedInstrs[CI]);
    }
  }
  return true;
}
/***
 * This function inserts a second complimentary branch instruction to be
 * checked
 * @param targetInstr Point at which the call should be inserted.
 * @param sucessorNum Successor number to replicate.
 * @param gdFunction Pointer to glitch detected function.
 * @return true if insertion is succesful else false
 */
bool insertBranch2(BranchInst &targetInstr, unsigned successorNum,
                   Function *gdFunction,
                   std::set<Instruction *> &insertedBranches) {
  bool retVal = true;

  try {
    // Only instrument conditional branches
    if (!targetInstr.isConditional()) {
      return false;
    }

    assert((successorNum == 0 || successorNum == 1) &&
           "Successor number cannot be other than 0 or 1");

    // if (Verbose)
    // {
    dbgs() << TAG << "Instrumenting:" << targetInstr << "\n";
    // }
    BasicBlock *targetBB = targetInstr.getParent();

    // Create a new basic block for our redundant check
    BasicBlock *doubleCheck =
        BasicBlock::Create(targetInstr.getContext(), "doubleCheck");
    // create a failure block
    BasicBlock *failBlock =
        BasicBlock::Create(targetInstr.getContext(), "failBlock");

    // Get the basicblock of the true branch
    auto trueBB = targetInstr.getSuccessor(successorNum);
    // we create a fall through BB so that we can update all the BBs
    // that refer to targetBB with fallThroughBB
    BasicBlock *fallthroughBB =
        BasicBlock::Create(targetInstr.getContext(), "fallThrough");
    IRBuilder<> builder(fallthroughBB);
    // connect fallthrough to the true BB
    builder.CreateBr(trueBB);

    // Insert our new BBs into the function
    fallthroughBB->insertInto(targetInstr.getFunction(), trueBB);
    doubleCheck->insertInto(targetInstr.getFunction(), fallthroughBB);
    failBlock->insertInto(targetInstr.getFunction(), fallthroughBB);

    // Update true branch to be doubleCheck
    targetInstr.setSuccessor(successorNum, doubleCheck);

    // Get the detection function
    Function *targetFunction = gdFunction;

    // Construct our redundant check and call to detection function
    builder.SetInsertPoint(doubleCheck);

    Instruction *branchNew = nullptr;
    if (successorNum == 0) {
      branchNew = builder.CreateCondBr(targetInstr.getCondition(),
                                       fallthroughBB, failBlock);
    } else {
      branchNew = builder.CreateCondBr(targetInstr.getCondition(), failBlock,
                                       fallthroughBB);
    }

    assert(branchNew != nullptr && "Branch instruction cannot be nullptr");

    // replicate the comparision activity
    std::vector<Instruction *> instrsToReplicate;
    instrsToReplicate.clear();
    getAllInstrToReplicate(targetInstr, instrsToReplicate);
    builder.SetInsertPoint(branchNew);
    duplicateInstructions(builder, *(dyn_cast<BranchInst>(branchNew)),
                          instrsToReplicate);

    insertedBranches.insert(dyn_cast<BranchInst>(branchNew));

    // Make our failure case call the glitch detected function
    builder.SetInsertPoint(failBlock);
    Instruction *callDetected = builder.CreateCall(targetFunction);
    builder.CreateBr(fallthroughBB); // so that the CFG is still complete

    // Now what we should do is in the trueBB.
    // replace all the PHIs that ref targetBB with fallThroughBB
    for (auto &currInstr : *trueBB) {
      Instruction *currInstrPtr = &currInstr;
      if (PHINode *phiInstr = dyn_cast<PHINode>(currInstrPtr)) {
        if (Verbose)
          dbgs() << TAG << "Fixing " << *phiInstr << "\n";
        int bbIndx = phiInstr->getBasicBlockIndex(targetBB);
        if (bbIndx >= 0) {
          Value *targetValue = phiInstr->getIncomingValue(bbIndx);
          phiInstr->removeIncomingValue(bbIndx);
          phiInstr->addIncoming(targetValue, fallthroughBB);
        }
      }
    }
    if (Verbose)
      dbgs() << TAG << "Inserted: \n" << *doubleCheck << "-----\n";
  } catch (const std::exception &e) {
    dbgs() << "[?] Error occurred while trying to instrument instruction:"
           << e.what() << "\n";
    retVal = false;
  }
  return retVal;
}