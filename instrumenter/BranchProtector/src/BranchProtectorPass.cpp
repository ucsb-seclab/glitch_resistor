//
// Created by machiry at the beginning of time.
//

// References
// LLVM Conditional branches:
// http://releases.llvm.org/2.6/docs/tutorial/JITTutorial2.html
//

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

#define GR_FUNC_NAME "gr_glitch_detected"

namespace GLitchPlease {

static cl::OptionCategory GPOptions("branchprotectorpass options");

// cl::opt<bool> Verbose("verbose",
//                       cl::desc("Print verbose information"),
//                       cl::init(false),
//                       cl::cat(GPOptions));

/***
 * The main pass.
 */
struct BranchProtectorPass : public FunctionPass {
public:
  static char ID;
  Function *grFunction;
  std::set<Function *> annotFuncs;
  std::string AnnotationString = "NoResistor";
  std::set<Instruction *> insertedBranches;
  std::string TAG = "\033[1;31m[GR/Branch]\033[0m ";
  bool Verbose = false;
  BranchProtectorPass() : FunctionPass(ID) { this->grFunction = nullptr; }

  Function *getGRFunction(Module &m) {
    if (this->grFunction == nullptr) {
      // void ()
      FunctionType *log_function_type =
          FunctionType::get(IntegerType::getVoidTy(m.getContext()), false);
      // get the reference to function
      Function *func = cast<Function>(
          m.getOrInsertFunction(GR_FUNC_NAME, log_function_type));

      this->grFunction = func;
    }
    return this->grFunction;
  }

  ~BranchProtectorPass() {}

  /**
   * Initialize our annotated functions set
   */
  virtual bool doInitialization(Module &M) override {
    getAnnotatedFunctions(&M);
    return false;
  }

  /**
   * Return false if this function was explicitly annoted to passed over
   */
  bool shouldInstrumentFunc(Function &F) {
    return annotFuncs.find(&F) == annotFuncs.end();
  }

  /**
   * Check to see if the branch insturction is one that was inserted by us
   */
  bool isInsertedBranch(Instruction &I) {
    return insertedBranches.find(&I) != insertedBranches.end();
  }

  /**
   * Get a list of all of the annotated funactions
   */
  void getAnnotatedFunctions(Module *M) {
    for (Module::global_iterator I = M->global_begin(), E = M->global_end();
         I != E; ++I) {

      if (I->getName() == "llvm.global.annotations") {
        ConstantArray *CA = dyn_cast<ConstantArray>(I->getOperand(0));
        for (auto OI = CA->op_begin(); OI != CA->op_end(); ++OI) {
          ConstantStruct *CS = dyn_cast<ConstantStruct>(OI->get());
          Function *FUNC = dyn_cast<Function>(CS->getOperand(0)->getOperand(0));
          GlobalVariable *AnnotationGL =
              dyn_cast<GlobalVariable>(CS->getOperand(1)->getOperand(0));
          StringRef annotation =
              dyn_cast<ConstantDataArray>(AnnotationGL->getInitializer())
                  ->getAsCString();
          if (annotation.compare(AnnotationString) == 0) {
            annotFuncs.insert(FUNC);
            // if (Verbose)
            // {
            //   dbgs() << "Found annotated function " << FUNC->getName() <<
            //   "\n";
            // }
          }
        }
      }
    }
  }

  /***
   * This function checks if the provided instruction can be replicated.
   * @param currIn Instruction to be checked.
   * @return true if can be replicate else false.
   */
  bool canReplicate(Instruction *currIn) {
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
  bool canReplicateOperands(Instruction *currIn) {
    // it should not be a call or a load or constant instruction.
    return !(dyn_cast<CallInst>(currIn) || dyn_cast<LoadInst>(currIn) ||
             dyn_cast<Constant>(currIn));
  }

  /***
   *  This function gets all the non-memory instructions that needed to
   * replicate to replicate the provided instruction.
   * @param currInstr Instruction that needed to replicate.
   * @param allInstrs list of instructions that needed to replicate.
   * @return true if there are any instructions that needed to replicate.
   */
  bool
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
  bool getAllInstrToReplicate(BranchInst &targetInstr,
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
  bool duplicateInstructions(IRBuilder<> &builder, BranchInst &targetBrInst,
                             std::vector<Instruction *> &allInstrs) {
    std::map<Instruction *, Instruction *> replicatedInstrs;
    for (auto currIn : allInstrs) {
      Instruction *newInstr = currIn->clone();
      if (ICmpInst *currICMPInstr = dyn_cast<ICmpInst>(currIn)) {
        CmpInst::Predicate currInstrP = currICMPInstr->getPredicate();
        if (currInstrP == CmpInst::ICMP_EQ || currInstrP == CmpInst::ICMP_NE) {
          assert (currICMPInstr->getNumOperands() == 2 && "Expect == and != to have 2 operands.");
          // first get the operands.
          Value *op1 = currICMPInstr->getOperand(0);
          Value *op2 = currICMPInstr->getOperand(1);

          // see if these are already replicated? if yes, get the replicated copies.
          if (Instruction *opInstr = dyn_cast<Instruction>(op1)) {
            if (replicatedInstrs.find(opInstr) != replicatedInstrs.end()) {
              op1 = replicatedInstrs[opInstr];
            }
          }

          if (Instruction *opInstr = dyn_cast<Instruction>(op2)) {
            if (replicatedInstrs.find(opInstr) != replicatedInstrs.end()) {
              op2 = replicatedInstrs[opInstr];
            }
          }

          // Now, negate them.
          Value *xorOp1 = builder.CreateBinOp(Instruction::BinaryOps::Xor, op1,
                                              ConstantInt::get(op1->getType(), ~0));
          Value *xorOp2 = builder.CreateBinOp(Instruction::BinaryOps::Xor, op2,
                                              ConstantInt::get(op2->getType(), ~0));

          // replace the operands with Xored operands.
          newInstr->replaceUsesOfWith(currICMPInstr->getOperand(0), xorOp1);
          newInstr->replaceUsesOfWith(currICMPInstr->getOperand(1), xorOp2);
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

      assert(newInstr != nullptr && "New instruction to insert cannot be nullptr");
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
   * @return true if insertion is succesful else false
   */
  bool insertBranch2(BranchInst &targetInstr) {
    bool retVal = true;

    try {
      // Only instrument conditional branches
      if (!targetInstr.isConditional()) {
        return false;
      }

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
      auto trueBB = targetInstr.getSuccessor(0);
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
      targetInstr.setSuccessor(0, doubleCheck);

      // Get the detection function
      Function *targetFunction = this->getGRFunction(*targetInstr.getModule());

      // Construct our redundant check and call to detection function
      builder.SetInsertPoint(doubleCheck);

      Instruction *branchNew = builder.CreateCondBr(targetInstr.getCondition(),
                                                    fallthroughBB, failBlock);
      // replicate the comparision activity
      std::vector<Instruction *> instrsToReplicate;
      instrsToReplicate.clear();
      getAllInstrToReplicate(targetInstr, instrsToReplicate);
      builder.SetInsertPoint(branchNew);
      duplicateInstructions(builder, *(dyn_cast<BranchInst>(branchNew)),
                            instrsToReplicate);

      // Keep track of the branches that we created so that we don't analyze
      // them again later.
      insertedBranches.insert(branchNew);

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

  /***
   * Check if the provided function is safe to modify.
   * @param currF Function to check.
   * @return flag that indicates if we can modify the current function or not.
   */
  bool isFunctionSafeToModify(const Function *currF) {
    return !(!currF->isDeclaration() && currF->hasName() &&
             currF->getName().equals(GR_FUNC_NAME));
  }

  /***
   * The function is the insertion oracle. This returns true
   * if delay has to be inserted at (before or after) the instruction.
   * @param currInstr Insruction before which the delay has to be inserted.
   * @param [Output] after Flag that indicates that the delay has to be inserted
   * after the currInstr.
   * @return true if the delay has to be inserted else false.
   */
  bool canInsertDelay(Instruction *currInstr, bool &after) {
    // TODO: fill this up with reasonable checks
    // as of now we insert delay before each switch statement.
    // but this can be changed.
    after = false;
    return dyn_cast<SwitchInst>(currInstr) != nullptr;
  }

  bool runOnFunction(Function &F) override {
    // Should we instrument this function?
    if (shouldInstrumentFunc(F) == false) {
      return false;
    }

    // Place a call to our delay function at the end of every basic block in the
    // function
    bool edited = false;
    errs() << TAG << F.getName() << "\n";

    if (isFunctionSafeToModify(&F)) {
      // errs() << TAG << F << "\n";
      for (auto &bb : F) {
        // dbgs() << TAG << bb << "\n";
        for (auto &ins : bb) {
          // errs() << TAG << ins << "\n";
          if (isa<BranchInst>(ins)) {
            if (isInsertedBranch(ins)) {
              // if (Verbose)
              // {
              //   dbgs() << TAG << "Skipping branch, it's one we inserted.\n";
              // }
              continue;
            }
            insertBranch2(cast<BranchInst>(ins));
          }
        }
        // insertDelay(&bb.back(), false);
        if (Verbose)
          dbgs() << TAG << bb << "\n";
      }
    } else {
      if (Verbose)
        errs() << TAG << "Cannot modify: " << F.getName() << "\n";
    }

    return edited;
  }
};

char BranchProtectorPass::ID = 0;
// pass arg, pass desc, cfg_only, analysis only
static RegisterPass<BranchProtectorPass>
    x("branchProtector",
      "Instrument all conditional branches to implement redudant checking",
      false, false);

// Pass loading stuff
// To use, run: clang -Xclang -load -Xclang <your-pass>.so <other-args> ...

// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const PassManagerBuilder &Builder,
                     llvm::legacy::PassManagerBase &PM) {
  PM.add(new BranchProtectorPass());
}
// These constructors add our pass to a list of global extensions.
static RegisterStandardPasses
    clangtoolLoader_Ox(PassManagerBuilder::EP_OptimizerLast, loadPass);
static RegisterStandardPasses
    clangtoolLoader_O0(PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);
} // namespace GLitchPlease
