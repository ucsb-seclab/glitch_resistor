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
    BasicBlock *hdr = currLoop->getHeader();
    for (auto it = pred_begin(hdr), et = pred_end(hdr); it != et; ++it)
    {
      BasicBlock* predecessor = *it;
      if (!currLoop->contains(predecessor))
      {
        enteringBBs.insert(predecessor);
      }
    }
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
      termIt--;
      builder.SetInsertPoint(&(*termIt));
    }
    Value *cons = ConstantInt::get(IntegerType::getInt32Ty(dstBB->getContext()), storeCons);
    builder.CreateStore(cons, dstPtr, true);
  }

  /***
   *  Reset the value of the provided loop variable with 0
   *
   * @param loopVar pointer to the local variable which needs to be reset.
   * @param targetBBs Blocks where the value should be reset.
   */
  void resetValue(Value* loopVar, std::set<BasicBlock*> &targetBBs)
  {
    // in each of the basic block..store 0
    for (auto currBB: targetBBs)
    {
      storeValue(0, loopVar, currBB);
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
   * @return True, if any instrumentation is done else false.
   */
  bool protectLoopExits(std::map<BasicBlock*, std::set<BasicBlock*>> &exitBBCorrespondence,
                        std::map<BasicBlock*, unsigned> &exitingBBCodes,
                        Value *loopVariable)
                        {

    LLVMContext &C = loopVariable->getContext();
    for (auto &exitBBCoL:exitBBCorrespondence)
    {
      BasicBlock *exitingBB = exitBBCoL.first;
      std::set<BasicBlock*> &inLoopBBs = exitBBCoL.second;

      // create a fall through block for the exiting bb
      BasicBlock *exitFallThrough = BasicBlock::Create(exitingBB->getContext(), "exitFallThrough");
      exitFallThrough->insertInto(exitingBB->getParent(), exitingBB);
      IRBuilder<> builder(exitFallThrough);
      // jump to the exiting bb
      builder.CreateBr(exitingBB);

      Value* loopLoadVal = nullptr;
      BasicBlock *firstCheck = nullptr;
      BasicBlock *prevBB = nullptr;
      Value *prevBBICmd = nullptr;

      // for each of the in loop basic blocks, create a comparision instruction
      for (auto *inLoopBB: inLoopBBs)
      {
        unsigned bbUniqueNumber = exitingBBCodes[inLoopBB];
        BasicBlock *cmpBB = BasicBlock::Create(C, "checkVal");
        cmpBB->insertInto(exitingBB->getParent(), exitFallThrough);
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
          inCmdBuilder.CreateCondBr(prevBBICmd, exitFallThrough, cmpBB);
        }

        prevBB = cmpBB;
        prevBBICmd = icmpEq;
      }

      // create loop check failed BB
      BasicBlock *terminatingBB = BasicBlock::Create(exitingBB->getContext(), "LoopCheckFailed");
      terminatingBB->insertInto(exitingBB->getParent(), exitFallThrough);
      builder.SetInsertPoint(terminatingBB);
      builder.CreateCall(getGRFunction(*(exitingBB->getModule())));
      builder.CreateBr(exitFallThrough);

      // link the last BB to the check failed BB
      builder.SetInsertPoint(prevBB);
      builder.CreateCondBr(prevBBICmd, exitFallThrough, terminatingBB);


      // update the targets of all BBs in inLoopBBs from exitingBB to firstCheck
      // instead of jumping to exit, jump to the checking chain.
      for (auto currILBB: inLoopBBs)
      {
        Instruction* termInst = currILBB->getTerminator();
        termInst->replaceUsesOfWith(exitingBB, firstCheck);
      }

      // finally update the PHI instructions in exitingBB that refer to one of the
      // inloop BB with exitFallThrough BB
      for (auto &currInstr: *exitingBB)
      {
        Instruction *currInstrPtr = &currInstr;
        if (PHINode *phiInstr = dyn_cast<PHINode>(currInstrPtr))
        {
          for (auto targetBB: inLoopBBs)
          {
            int bbIndx = phiInstr->getBasicBlockIndex(targetBB);
            if (bbIndx >= 0)
            {
              Value *targetValue = phiInstr->getIncomingValue(bbIndx);
              phiInstr->removeIncomingValue(bbIndx);
              phiInstr->addIncoming(targetValue, exitFallThrough);
            }
          }
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

    bool edited = false;
    errs() << TAG << "Instrumenting: " << F.getName() << "!\n";

    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    if (isFunctionSafeToModify(&F))
    {
      // get all the loops in the function.
      for (auto *lobj: LI.getLoopsInPreorder())
      {
        // for each loop create a new local variable.
        Value *loopVar = createNewLocalVariable(F);

        // before entering the loop..set the value of the variable to 0
        std::set<BasicBlock*> entryBBs;
        entryBBs.clear();
        getEnteringBBs(lobj, entryBBs);
        assert(!entryBBs.empty() && "There has to be en try BBs.");
        resetValue(loopVar, entryBBs);

        SmallVector<BasicBlock *, 32> exitBBs;
        exitBBs.clear();
        // get the exit basic blocks.
        lobj->getExitingBlocks(exitBBs);

        std::vector<unsigned> solomonCodes;
        solomonCodes.clear();
        // get solomon codes for each of the exiting BBs
        generateNumbersWithMaximumHamming(exitBBs.size(), solomonCodes);
        assert(exitBBs.size() == solomonCodes.size()  && "Unable to get required number of solomon codes.");
        std::map<BasicBlock*, unsigned> exitingBBCodes;
        exitingBBCodes.clear();
        unsigned solIdx = 0;

        // assign one code for each of the exiting BB
        for (auto currExitBB: exitBBs)
        {
          exitingBBCodes[currExitBB] = solomonCodes[solIdx];
          // store the corresponding value into the loop local var.
          storeValue(solomonCodes[solIdx], loopVar, currExitBB);
          solIdx++;
        }

        // get the correspondence of exiting loop bbs
        std::map<BasicBlock*, std::set<BasicBlock*>> exitBBCorrespondence;
        exitBBCorrespondence.clear();
        getExitBBCorrespondence(lobj, exitBBCorrespondence);

        // protect the loop exits.
        protectLoopExits(exitBBCorrespondence, exitingBBCodes, loopVar);
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