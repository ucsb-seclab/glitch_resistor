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

using namespace llvm;

#define DELAY_FUNC_NAME "gpdelay"

namespace GLitchPlease {


  static cl::OptionCategory GPOptions("delayinjectorpass options");

  cl::opt<bool> Verbose("verbose",
                        cl::desc("Print verbose information"),
                        cl::init(false),
                        cl::cat(GPOptions));

  /***
   * The main pass.
   */
  struct DelayInjectorPass : public FunctionPass {
  public:
    static char ID;
    Function *delayFunction;
    std::set<Function*> annotFuncs;
    std::string AnnotationString = "NoResistor";

    DelayInjectorPass() : FunctionPass(ID) {
      this->delayFunction = nullptr;
    }

    Function *getDelayFunction(Module &m) {
      if (this->delayFunction == nullptr) {
        // void ()
        FunctionType *log_function_type = FunctionType::get(IntegerType::getVoidTy(m.getContext()),false);
        // get the reference to function
        Function *func = cast<Function>(m.getOrInsertFunction(DELAY_FUNC_NAME, log_function_type));

        this->delayFunction = func;
      }
      return this->delayFunction;
    }


    ~DelayInjectorPass() {
    }


    virtual bool doInitialization(Module &M)override{
        getAnnotatedFunctions(&M);
        return false;
    }
    /**
     * Return false if this function was explicitly annoted to passed over
     */
    bool shouldInstrumentFunc(Function &F){
        return annotFuncs.find(&F)==annotFuncs.end();
    }

    /** 
     * Get a list of all of the annotated functions
     */
    void getAnnotatedFunctions(Module *M){
        for (Module::global_iterator I = M->global_begin(),
                E = M->global_end();
                I != E;
                ++I) {

            if (I->getName() == "llvm.global.annotations") {
                ConstantArray *CA = dyn_cast<ConstantArray>(I->getOperand(0));
                for(auto OI = CA->op_begin(); OI != CA->op_end(); ++OI){
                    ConstantStruct *CS = dyn_cast<ConstantStruct>(OI->get());
                    Function *FUNC = dyn_cast<Function>(CS->getOperand(0)->getOperand(0));
                    GlobalVariable *AnnotationGL = dyn_cast<GlobalVariable>(CS->getOperand(1)->getOperand(0));
                    StringRef annotation = dyn_cast<ConstantDataArray>(AnnotationGL->getInitializer())->getAsCString();
                    if(annotation.compare(AnnotationString)==0){
                        annotFuncs.insert(FUNC);
                        errs() << "Found annotated function " << FUNC->getName()<<"\n";
                    }
                }
            }
        }
    }

    /***
     * This function inserts call to a delay function at the provided instruction.
     * @param targetInstr Point at which the call should be inserted.
     * @param insertAfter flag to indicate that call should be inserted after the instruction.
     * @return true if insertion is succesful else false
     */
    bool insertDelay(Instruction *targetInstr, bool insertAfter = false) {
      bool retVal = true;

      try {
        if(Verbose) {
          dbgs() << "Instrumenting:" << *targetInstr << "\n";
        }
        // set the insertion point to be after the load instruction.
        auto targetInsertPoint = targetInstr->getIterator();
        if(insertAfter) {
          targetInsertPoint++;
        }
        IRBuilder<> builder(&(*targetInsertPoint));

        // get the log function
        Function *targetFunction = this->getDelayFunction(*targetInstr->getModule());

        // create call.
        builder.CreateCall(targetFunction);
      } catch (const std::exception &e) {
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
    bool isFunctionSafeToModify(const Function *currF) {
      return !(!currF->isDeclaration() && currF->hasName() && currF->getName().equals(DELAY_FUNC_NAME));
    }


    /***
     * The function is the insertion oracle. This returns true
     * if delay has to be inserted at (before or after) the instruction.
     * @param currInstr Insruction before which the delay has to be inserted.
     * @param [Output] after Flag that indicates that the delay has to be inserted after
     *                       the currInstr.
     * @return true if the delay has to be inserted else false.
     */
    bool canInsertDelay(Instruction *currInstr, bool &after) {
      //TODO: fill this up with reasonable checks
      // as of now we insert delay before each switch statement.
      // but this can be changed.
      after = false;
      return dyn_cast<SwitchInst>(currInstr) != nullptr;
    }


    bool runOnFunction(Function &F) override {
      if(shouldInstrumentFunc(F)==false) {
            return false;
      }

      bool edited = false;
      errs() << "I saw a function called " << F.getName() << "!\n";
      if(isFunctionSafeToModify(&F)) {
        for (auto &bb: F) {
          errs() << "I saw a bb called " << bb.getName() << "!\n";
          insertDelay(&bb.back(), false);
          // for (auto &instr: bb) {
            
          //   // this is the current instruction.
          //   Instruction *currInstr = &instr;
          //   bool after;
          //   if(canInsertDelay(currInstr, after)) {
          //     insertDelay(currInstr, after);
          //   }
          // }
        }
      }

      return edited;
    }

  };

  char DelayInjectorPass::ID = 0;
  // pass arg, pass desc, cfg_only, analysis only
  static RegisterPass<DelayInjectorPass> x("injectDelay",
                                            "Instrument the provided module by "
                                            "inserting call to functions that cause random delays.",
                                            false,
                                            false);

  // Pass loading stuff
  // To use, run: clang -Xclang -load -Xclang <your-pass>.so <other-args> ...

  // This function is of type PassManagerBuilder::ExtensionFn
  static void loadPass(const PassManagerBuilder &Builder, llvm::legacy::PassManagerBase &PM) {
    PM.add(new DelayInjectorPass());
  }
  // These constructors add our pass to a list of global extensions.
  static RegisterStandardPasses clangtoolLoader_Ox(PassManagerBuilder::EP_OptimizerLast, loadPass);
  static RegisterStandardPasses clangtoolLoader_O0(PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);
}