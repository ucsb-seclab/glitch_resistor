//
// Created by machiry at the beginning of time.
//

// References
// LLVM Conditional branches: http://releases.llvm.org/2.6/docs/tutorial/JITTutorial2.html
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

using namespace llvm;

#define GR_FUNC_NAME "gr_glitch_detected"

namespace GLitchPlease
{

static cl::OptionCategory GPOptions("branchprotectorpass options");

// cl::opt<bool> Verbose("verbose",
//                       cl::desc("Print verbose information"),
//                       cl::init(false),
//                       cl::cat(GPOptions));

/***
   * The main pass.
   */
struct BranchProtectorPass : public FunctionPass
{
public:
  static char ID;
  Function *grFunction;
  std::set<Function *> annotFuncs;
  std::string AnnotationString = "NoResistor";
  std::set<Instruction *> insertedBranches;
  std::string TAG = "\033[1;31m[GR/Branch]\033[0m ";

  BranchProtectorPass() : FunctionPass(ID)
  {
    this->grFunction = nullptr;
  }

  Function *getDelayFunction(Module &m)
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

  ~BranchProtectorPass()
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
      * Check to see if the branch insturction is one that was inserted by us
     */
  bool isInsertedBranch(Instruction &I)
  {
    return insertedBranches.find(&I) != insertedBranches.end();
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

  CmpInst *getCmpInst(Instruction &ins)
  {
    Instruction *prevInst = ins.getPrevNonDebugInstruction();
    while (!isa<CmpInst>(prevInst) && prevInst != NULL)
    {
      dbgs() << TAG << *prevInst << "\n";
      prevInst = prevInst->getPrevNonDebugInstruction();
    }
    if (prevInst == NULL)
    {
      return NULL;
    }
    else
    {
      return cast<CmpInst>(prevInst);
    }
  }
  /***
     * This function inserts a second complimentary branch instruction to be checked
     * @param targetInstr Point at which the call should be inserted.
     * @return true if insertion is succesful else false
     */
  bool insertBranch2(BranchInst &targetInstr)
  {
    bool retVal = true;

    try
    {
      // Only instrument conditional branches
      if (!targetInstr.isConditional())
      {
        return false;
      }
      // if (Verbose)
      // {
      dbgs() << TAG << "Instrumenting:" << targetInstr << "\n";
      // }

      // // Get our compare instruction
      // CmpInst *cmpInstruction = getCmpInst(targetInstr);

      // dbgs() << TAG << *cmpInstruction << "\n";
      // Create a new basic block for our redundant check
      BasicBlock *doubleCheck = BasicBlock::Create(targetInstr.getContext(), "doubleCheck");
      BasicBlock *failBlock = BasicBlock::Create(targetInstr.getContext(), "failBlock");

      // Get the basicblock of the true branch
      auto trueBB = targetInstr.getSuccessor(0);

      // Insert our new BBs into the function
      doubleCheck->insertInto(targetInstr.getFunction(), trueBB);
      failBlock->insertInto(targetInstr.getFunction(), doubleCheck);

      // Update true branch to be doubleCheck
      targetInstr.setSuccessor(0, doubleCheck);

      // Get the detection function
      Function *targetFunction = this->getDelayFunction(*targetInstr.getModule());

      // Construct our redudant check and call to detection function
      IRBuilder<> builder(doubleCheck);

      // TODO: Do some manipulation to the value and change the comparison

      // New comparison and branch (going to detection function if it fails)
      // Value *cmpCond = builder.CreateICmp(cmpInstruction->getPredicate(),
      //                                     cmpInstruction->getOperand(0),
      //                                     cmpInstruction->getOperand(1));
      Instruction *branchNew = builder.CreateCondBr(targetInstr.getCondition(),
                                                    trueBB,
                                                    failBlock);

      // Keep track of the branches that we created so that we don't analyze them again later.
      insertedBranches.insert(branchNew);

      // Make our failure case call the glitch detected function
      builder.SetInsertPoint(failBlock);
      Instruction *callDetected = builder.CreateCall(targetFunction);
      builder.CreateBr(trueBB); // so that the CFG is still complete
    }
    catch (const std::exception &e)
    {
      dbgs() << "[?] Error occurred while trying to instrument instruction:" << e.what() << "\n";
      retVal = false;
    }
    return retVal;
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

  /***
     * The function is the insertion oracle. This returns true
     * if delay has to be inserted at (before or after) the instruction.
     * @param currInstr Insruction before which the delay has to be inserted.
     * @param [Output] after Flag that indicates that the delay has to be inserted after
     *                       the currInstr.
     * @return true if the delay has to be inserted else false.
     */
  bool canInsertDelay(Instruction *currInstr, bool &after)
  {
    //TODO: fill this up with reasonable checks
    // as of now we insert delay before each switch statement.
    // but this can be changed.
    after = false;
    return dyn_cast<SwitchInst>(currInstr) != nullptr;
  }

  bool runOnFunction(Function &F) override
  {
    // Should we instrument this function?
    if (shouldInstrumentFunc(F) == false)
    {
      return false;
    }

    // Place a call to our delay function at the end of every basic block in the function
    bool edited = false;
    errs() << TAG << "Instrumenting: " << F.getName() << "!\n";

    if (isFunctionSafeToModify(&F))
    {
      // errs() << TAG << F << "\n";
      for (auto &bb : F)
      {
        // errs() << TAG << bb << "\n";
        for (auto &ins : bb)
        {
          // errs() << TAG << ins << "\n";
          if (isa<BranchInst>(ins))
          {
            if (isInsertedBranch(ins))
            {
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
      }
    }

    return edited;
  }
};

char BranchProtectorPass::ID = 0;
// pass arg, pass desc, cfg_only, analysis only
static RegisterPass<BranchProtectorPass> x("branchProtector",
                                           "Instrument all conditional branches to implement redudant checking",
                                           false,
                                           false);

// Pass loading stuff
// To use, run: clang -Xclang -load -Xclang <your-pass>.so <other-args> ...

// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const PassManagerBuilder &Builder, llvm::legacy::PassManagerBase &PM)
{
  PM.add(new BranchProtectorPass());
}
// These constructors add our pass to a list of global extensions.
static RegisterStandardPasses clangtoolLoader_Ox(PassManagerBuilder::EP_OptimizerLast, loadPass);
static RegisterStandardPasses clangtoolLoader_O0(PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);
} // namespace GLitchPlease