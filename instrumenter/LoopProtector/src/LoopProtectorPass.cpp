//
// Created by machiry at the beginning of time.
//

#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <iostream>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Support/Debug.h>
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <set>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include "Utils.h"

using namespace llvm;

#define GR_FUNC_NAME "gr_glitch_detected"

namespace GLitchPlease
{

static cl::OptionCategory GPOptions("loopprotectorpass options");

cl::opt<bool> Verbose("verbose",
                       cl::desc("Print verbose information"),
                       cl::init(false),
                       cl::cat(GPOptions));

/***
   * The main pass.
   */
struct LoopProtectorPass : public FunctionPass
{
public:
  static char ID;
  Function *grFunction;
  std::set<Function *> annotFuncs;
  std::string AnnotationString = "NoResistor";
  std::string TAG = "\033[1;31m[GR/Loop]\033[0m ";

  LoopProtectorPass() : FunctionPass(ID)
  {
    this->grFunction = nullptr;
  }

  Function *getGRFunction(Module &m)
  {
    if (this->grFunction == nullptr)
    {
      // void ()
      FunctionType *log_function_type = FunctionType::get(IntegerType::getVoidTy(m.getContext()), false);
      // get the reference to function
      Function *func = cast<Function>(m.getOrInsertFunction(GR_FUNC_NAME, log_function_type));

      this->grFunction = func;
    }
    return this->grFunction;
  }

  ~LoopProtectorPass()
  {
  }

  /**
     * Initialize our annotated functions set
     */
  virtual bool doInitialization(Module &M) override
  {
    getAnnotatedFunctions(&M);
    return false;
  }

  /**
     * Return false if this function was explicitly annoted to passed over
     */
  bool shouldInstrumentFunc(Function &F)
  {
    return annotFuncs.find(&F) == annotFuncs.end();
  }

  /** 
     * Get a list of all of the annotated functions
     */
  void getAnnotatedFunctions(Module *M)
  {
    for (Module::global_iterator I = M->global_begin(),
                                 E = M->global_end();
         I != E;
         ++I)
    {

      if (I->getName() == "llvm.global.annotations")
      {
        ConstantArray *CA = dyn_cast<ConstantArray>(I->getOperand(0));
        for (auto OI = CA->op_begin(); OI != CA->op_end(); ++OI)
        {
          ConstantStruct *CS = dyn_cast<ConstantStruct>(OI->get());
          Function *FUNC = dyn_cast<Function>(CS->getOperand(0)->getOperand(0));
          GlobalVariable *AnnotationGL = dyn_cast<GlobalVariable>(CS->getOperand(1)->getOperand(0));
          StringRef annotation = dyn_cast<ConstantDataArray>(AnnotationGL->getInitializer())->getAsCString();
          if (annotation.compare(AnnotationString) == 0)
          {
            annotFuncs.insert(FUNC);
            // if (Verbose)
            // {
            //   dbgs() << "Found annotated function " << FUNC->getName() << "\n";
            // }
          }
        }
      }
    }
  }

  /***
     * Check if the provided function is safe to modify.
     * @param currF Function to check.
     * @return flag that indicates if we can modify the current function or not.
     */
  bool isFunctionSafeToModify(const Function *currF)
  {
    return !(!currF->isDeclaration() && currF->hasName() && currF->getName().equals(GR_FUNC_NAME));
  }

  /**
   * Required to enabel loop analysis on function pass
   * Ref: https://stackoverflow.com/questions/30351725/llvm-loopinfo-in-functionpass-doesnt-compile
   */
  void getAnalysisUsage(AnalysisUsage &AU) const override
  {
    AU.setPreservesCFG();
    AU.addRequired<LoopInfoWrapperPass>();
  }

  /***
   * Create a new local integer variable at the entry of the function.
   * @param F Function in which the local variable needs to be created.
   * @return Pointer to the created local variable.
   */
  Value* createNewLocalVariable(Function &F)
  {
    IRBuilder<> builder(&(*(F.getEntryBlock().getFirstInsertionPt())));
    AllocaInst *newAlloca = builder.CreateAlloca(IntegerType::getInt32Ty(F.getContext()), nullptr, "GPLoopVar");
    newAlloca->setAlignment(4);
    // initialize the loop var to zero.
    storeValue(0, newAlloca, &F.getEntryBlock());
    return newAlloca;
  }

  /***
   *  Get all the basic-blocks that are predecessors to the loops header
   *  i.e., these are the basic-blocks immediately precede the loop.
   * @param currLoop Target loop.
   * @param enteringBBs Set where the pointers to the basic blocks should be stored.
   * @return true if there are some basic blocks
   */
  bool getEnteringBBs(Loop* currLoop, std::set<BasicBlock*> &enteringBBs)
  {
    enteringBBs.insert(currLoop->getHeader());
    return !enteringBBs.empty();
  }

  /***
   * Create a store instruction to store given constant into the
   * provided variable.
   *
   * @param storeCons Constant value to be stored.
   * @param dstPtr Pointer to the local variable.
   * @param dstBB Basic block where store instruction should be created
   */
  void storeValue(unsigned storeCons, Value *dstPtr, BasicBlock *dstBB)
  {
    IRBuilder<> builder(&(*dstBB->getFirstInsertionPt()));
    if (dstBB->getInstList().size() > 1)
    {
      auto termIt = dstBB->getTerminator()->getIterator();
      builder.SetInsertPoint(&(*termIt));
    }
    Value *cons = ConstantInt::get(IntegerType::getInt32Ty(dstBB->getContext()), storeCons);
    builder.CreateStore(cons, dstPtr, true);
  }

  /***
   *  Reset the value of the provided loop variable with resetVal
   *
   * @param loopVar pointer to the local variable which needs to be reset.
   * @param resetVal Value which should be written into the provided loopVar
   * @param targetBBs Blocks where the value should be reset.
   */
  void resetValue(Value* loopVar, unsigned resetVal, std::set<BasicBlock*> &targetBBs)
  {
    // in each of the basic block..store 0
    for (auto currBB: targetBBs)
    {
      storeValue(resetVal, loopVar, currBB);
    }
  }

  /***
   *  Get the correspondence between the loop exit basic blocks
   *  and corresponding in loop exiting basic blocks.
   *
   * @param lo Loop object.
   * @param co Map that contains the correspondence between exit and corresponding exiting basic blocks.
   * @return true if there are non-zero exit basic blocks.
   */
  bool getExitBBCorrespondence(Loop *lo, std::map<BasicBlock*, std::set<BasicBlock*>> &co)
  {
    SmallVector<Loop::Edge, 32> exitEdges;
    exitEdges.clear();
    lo->getExitEdges(exitEdges);
    for (auto &currEdge: exitEdges)
    {
      BasicBlock *outBB = const_cast<BasicBlock*>(currEdge.second);
      BasicBlock *inBB = const_cast<BasicBlock*>(currEdge.first);
      co[outBB].insert(inBB);
    }
    return !co.empty();
  }


  /***
   *  Protect the provided loops by editing the loop exit basic blocks
   *  by checking that the loop exited from expected basic blocks.
   *  i.e.,
   *   if loopv == val1 || loopv == val2 .. successful
   *   else fail
   *
   * @param exitBBCorrespondence Map between loop exit basic block and
   *                             corresponding in loop basic blocks.
   * @param exitingBBCodes Map of in loop basic block and corresponding
   *                       numerical codes.
   * @param loopVariable Variable which contains the value corresponding to the
   *                     exit of the loop.
   * @param uniqueLoopNumber a number that indicates that control flow has
   *                         entered the loop.
   * @return True, if any instrumentation is done else false.
   */
  bool protectLoopExits(std::map<BasicBlock*, std::set<BasicBlock*>> &exitBBCorrespondence,
                        std::map<BasicBlock*, unsigned> &exitingBBCodes,
                        Value *loopVariable, unsigned uniqueLoopNumber, std::map<BasicBlock*, BasicBlock*> &splittedBBMap)
                        {

    LLVMContext &C = loopVariable->getContext();
    for (auto &exitBBCoL:exitBBCorrespondence)
    {
      BasicBlock *originalExitingBB = exitBBCoL.first;
      std::set<BasicBlock*> &inLoopBBs = exitBBCoL.second;

      Instruction *exitingSplitInstr = nullptr;

      // first, split the exiting BB after all the PHI instructions.
      for (auto &currInstr: *originalExitingBB)
      {
        Instruction *currInstrPtr = &currInstr;
        if (!dyn_cast<PHINode>(currInstrPtr))
        {
          exitingSplitInstr = currInstrPtr;
          break;
        }
      }

      if(exitingSplitInstr != nullptr) {
        // split the block at the splitting instruction.
        BasicBlock *newExitBB = SplitBlock(originalExitingBB, exitingSplitInstr);
        BasicBlock *oldAllGoodBlock = nullptr;
        if(splittedBBMap.find(originalExitingBB) == splittedBBMap.end()) {
          splittedBBMap[originalExitingBB] = newExitBB;
        }
        oldAllGoodBlock = splittedBBMap[originalExitingBB];

        // create a fall through block for the exiting bb
        BasicBlock *exitFallThrough = BasicBlock::Create(originalExitingBB->getContext(), "exitFallThrough");
        exitFallThrough->insertInto(originalExitingBB->getParent(), originalExitingBB);
        IRBuilder<> builder(exitFallThrough);
        // jump to the newly splitted exit bb
        builder.CreateBr(newExitBB);

        Value* loopLoadVal = nullptr;
        BasicBlock *firstCheck = nullptr;
        BasicBlock *prevBB = nullptr;
        Value *prevBBICmd = nullptr;

        // for each of the in loop basic blocks, create a comparision instruction
        for (auto *inLoopBB: inLoopBBs)
        {
          unsigned bbUniqueNumber = exitingBBCodes[inLoopBB];
          BasicBlock *cmpBB = BasicBlock::Create(C, "checkVal");
          cmpBB->insertInto(originalExitingBB->getParent(), exitFallThrough);
          IRBuilder<> inCmdBuilder(cmpBB);
          // is this first bb in the chain of comparisons
          if (firstCheck == nullptr)
          {
            loopLoadVal = inCmdBuilder.CreateLoad(loopVariable);
            firstCheck = cmpBB;
          }
          assert(loopLoadVal != nullptr && "Load value cannot be null.");
          Value *cons = ConstantInt::get(IntegerType::getInt32Ty(C), bbUniqueNumber);
          // create a comparision instruction.
          Value *icmpEq = inCmdBuilder.CreateICmpEQ(loopLoadVal, cons);

          if (prevBB != nullptr)
          {
            // link to the previous basic-block
            inCmdBuilder.SetInsertPoint(prevBB);
            inCmdBuilder.CreateCondBr(prevBBICmd, oldAllGoodBlock, cmpBB);
          }

          prevBB = cmpBB;
          prevBBICmd = icmpEq;
        }

        // create loop check failed BB
        BasicBlock *terminatingBB = BasicBlock::Create(originalExitingBB->getContext(), "LoopCheckFailed");
        terminatingBB->insertInto(originalExitingBB->getParent(), exitFallThrough);
        builder.SetInsertPoint(terminatingBB);
        builder.CreateCall(getGRFunction(*(originalExitingBB->getModule())));
        builder.CreateBr(exitFallThrough);


        // here we check whether the control flow actually entered the loop or not?
        // i.e., if the value of loopVar != uniqueLoopNumber that means the control
        // did not entered the loop.
        BasicBlock *loopEntryCheck = BasicBlock::Create(originalExitingBB->getContext(), "checkLoopEntry");
        loopEntryCheck->insertInto(originalExitingBB->getParent(), exitFallThrough);
        builder.SetInsertPoint(loopEntryCheck);
        Value *cons = ConstantInt::get(IntegerType::getInt32Ty(C), uniqueLoopNumber);
        // create a not equal comparision instruction.
        Value *icmpNEq = builder.CreateICmpNE(loopLoadVal, cons);
        // control did not enter the loop, so this is fine.
        builder.CreateCondBr(icmpNEq, exitFallThrough, terminatingBB);

        // link the last BB to the check failed BB
        builder.SetInsertPoint(prevBB);
        // branch of check if the control actually entered the loop or not.
        builder.CreateCondBr(prevBBICmd, oldAllGoodBlock, loopEntryCheck);

        // update the original exit BB so that it jumps to our
        // series of newly inserted checks
        Instruction* termInst = originalExitingBB->getTerminator();
        termInst->replaceUsesOfWith(newExitBB, firstCheck);

      } else {
        if(Verbose) {
          errs() << "Not instrumenting loop as we cannot split exiting basic block:";
          originalExitingBB->dump();
        }
      }
    }
    return true;
  }

  bool runOnFunction(Function &F) override
  {
    // Should we instrument this function?
    if (shouldInstrumentFunc(F) == false)
    {
      return false;
    }

    srand(time(NULL));

    bool edited = false;
    errs() << TAG << "Instrumenting: " << F.getName() << "!\n";

    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    std::map<BasicBlock*, BasicBlock*> splittedBBMap;
    splittedBBMap.clear();

    if (isFunctionSafeToModify(&F))
    {
      // get all the loops in the function.
      for (auto *lobj: LI.getLoopsInPreorder())
      {
        // for each loop create a new local variable.
        Value *loopVar = createNewLocalVariable(F);
        // before entering the loop..set the value of the variable
        // to a random value that indicates that the loop has been entered.
        // this is a unique number that represents that control has entered the loop
        unsigned uniqueLoopNumber = (unsigned)rand();
        std::set<BasicBlock*> entryBBs;
        entryBBs.clear();
        getEnteringBBs(lobj, entryBBs);
        assert(!entryBBs.empty() && "There has to be entry BBs.");
        resetValue(loopVar, uniqueLoopNumber,entryBBs);

        SmallVector<BasicBlock *, 32> exitBBs;
        exitBBs.clear();
        // get the exit basic blocks.
        lobj->getExitingBlocks(exitBBs);

        /*solomonCodes.clear();
        // get solomon codes for each of the exiting BBs
        generateNumbersWithMaximumHamming(exitBBs.size(), solomonCodes);
        assert(exitBBs.size() == solomonCodes.size()  && "Unable to get required number of solomon codes.");*/
        std::map<BasicBlock*, unsigned> exitingBBCodes;
        exitingBBCodes.clear();
        unsigned solIdx = 0;

        // assign one code for each of the exiting BB
        for (auto currExitBB: exitBBs)
        {
          exitingBBCodes[currExitBB] = (unsigned)rand();
          // store the corresponding value into the loop local var.
          storeValue(exitingBBCodes[currExitBB], loopVar, currExitBB);
          solIdx++;
        }

        // get the correspondence of exiting loop bbs
        std::map<BasicBlock*, std::set<BasicBlock*>> exitBBCorrespondence;
        exitBBCorrespondence.clear();
        getExitBBCorrespondence(lobj, exitBBCorrespondence);

        // protect the loop exits.
        protectLoopExits(exitBBCorrespondence, exitingBBCodes, loopVar, uniqueLoopNumber, splittedBBMap);
      }
    }

    return edited;
  }
};

char LoopProtectorPass::ID = 0;
// pass arg, pass desc, cfg_only, analysis only
static RegisterPass<LoopProtectorPass> x("loopProtector",
                                         "Ensure that all loops are properly terminated.",
                                         false,
                                         false);

// Pass loading stuff
// To use, run: clang -Xclang -load -Xclang <your-pass>.so <other-args> ...

// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const PassManagerBuilder &Builder, llvm::legacy::PassManagerBase &PM)
{
  PM.add(new LoopProtectorPass());
}
// These constructors add our pass to a list of global extensions.
static RegisterStandardPasses clangtoolLoader_Ox(PassManagerBuilder::EP_OptimizerLast, loadPass);
static RegisterStandardPasses clangtoolLoader_O0(PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);
} // namespace GLitchPlease