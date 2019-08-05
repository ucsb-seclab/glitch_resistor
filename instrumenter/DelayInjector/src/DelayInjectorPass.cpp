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
  struct DelayInjectorPass : public ModulePass {
  public:
    static char ID;
    Function *delayFunction;

    DelayInjectorPass() : ModulePass(ID) {
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


    bool runOnModule(Module &m) override {
      bool edited = false;
      //iterate through each function.
      for(auto &currF: m) {
        if(isFunctionSafeToModify(&currF)) {
          for (auto &bb: currF) {
            for (auto &instr: bb) {
              // this is the current instruction.
              Instruction *currInstr = &instr;
              bool after;
              if(canInsertDelay(currInstr, after)) {
                insertDelay(currInstr, after);
              }
            }
          }
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
}