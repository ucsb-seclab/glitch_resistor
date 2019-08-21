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

using namespace llvm;

#define GR_FUNC_NAME "gr_glitch_detected"

namespace GLitchPlease
{

static cl::OptionCategory GPOptions("loopprotectorpass options");

// cl::opt<bool> Verbose("verbose",
//                       cl::desc("Print verbose information"),
//                       cl::init(false),
//                       cl::cat(GPOptions));

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

    // LoopInfo *LI = &getAnalysis<LoopInfo>(F);
    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    // errs() << TAG << LI << "\n";
    if (isFunctionSafeToModify(&F))
    {
      errs() << TAG << F << "\n";
      for (auto &bb : F)
      {
        bool isLoop = LI.getLoopFor(&bb);
        if (isLoop)
        {
          errs() << "loop{\n";
          for (auto &ins : bb)
          {
            errs() << TAG << ins << "\n";
            if (isa<BranchInst>(ins))
            {
            }
          }
          errs() << "}\n";
        }
        // // errs() << TAG << bb << "\n";
        // for (auto &ins : bb)
        // {
        //   // errs() << TAG << ins << "\n";
        //   if (isa<BranchInst>(ins))
        //   {
        //   }
        // }
        // insertDelay(&bb.back(), false);
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