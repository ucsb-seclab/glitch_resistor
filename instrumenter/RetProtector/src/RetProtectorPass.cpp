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
#include <vector>

using namespace llvm;
using namespace std;

#define DELAY_FUNC_NAME "gr_glitch_detected"

namespace GLitchPlease {


  static cl::OptionCategory GPOptions("retprotector options");

  cl::opt<bool> Verbose("verbose",
                        cl::desc("Print verbose information"),
                        cl::init(false),
                        cl::cat(GPOptions));

  /***
   * The main pass.
   */
  struct RetProtectorPass : public ModulePass {
  public:
    static char ID;
    Function *delayFunction;
    std::set<Function*> annotFuncs;
    std::string AnnotationString = "NoResistor";

    RetProtectorPass() : ModulePass(ID) {
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


    ~RetProtectorPass() {
    }

    /**
     * Initialize our annotated functions set
     */
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
                        if(Verbose) {
                          dbgs() << "Found annotated function " << FUNC->getName()<<"\n";
                        }
                    }
                }
            }
        }
    }

    /***
     * This function will insert a call to the glitch_detected function after the instruction
     * @param targetInstr Point at which the call should be inserted.
     * @param insertAfter flag to indicate that call should be inserted after the instruction.
     * @return true if insertion is succesful else false
     */
    bool insertProtector(Instruction *targetInstr, bool insertAfter = true) {
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

    bool runOnModule(Module &M) override {
      // Place a call to our detection function after the return of every function
      bool edited = false;
     
      //TODO: other bit sizes as well, but 32 seems to be the most common      
      ConstantInt *zero = ConstantInt::get(Type::getInt32Ty(M.getContext()), 0);

      //TODO: Will likely need to handle invokes as well
      vector<ReturnInst *> rets;
      vector<CallInst *> calls;

      M.dump();
      errs() << "----------------------------------\n";
      //Find all returns of a constant 0
      for (User *U : zero->users()){
          Instruction *inst = dyn_cast<Instruction>(U);
          if(!inst || !shouldInstrumentFunc(*(inst->getFunction()))) continue;
          else if(ReturnInst *ri = dyn_cast<ReturnInst>(inst)) {
             //check if address taken 
            //ri->dump();
            //const User *zed;
            //errs() << "Address Taken: " << ri->getFunction()->hasAddressTaken(&zed) << "\n";
            //const BitCastOperator *tmp = dyn_cast<BitCastOperator>(zed);
            //if(tmp) {
            //  tmp->dump();
            //  for(const User *U2 : tmp->users())
            //    U2->dump();
            //}
            //if(!ri->getFunction()->hasAddressTaken()) 
              rets.push_back(ri); 
          }
      }

      //Find All call instructions in the Module
      for(auto it = M.begin(); it != M.end(); it++){
        Function &F = *it;
        for(BasicBlock &BB : F){
          for(Instruction &I : BB){
            Instruction *inst = &I;
            if(CallInst *ci = dyn_cast<CallInst>(inst)){
              if (ci->getCalledFunction())
                calls.push_back(ci);
            }
          }
        }
      }

      errs() << "number of constant 0 returns: " << rets.size() << "\n";
      errs() << "number of call instructions: " << calls.size() << "\n";
      
      //For each return instruction, verify that 1) it is in a modifiable
      //function and 2) *every* call to it is in a modifiable function.
      vector<ReturnInst *> mod_rets;
      for(ReturnInst *ri : rets){
        bool mod = true;
        StringRef ret_name = ri->getFunction()->getName();
        for(CallInst *ci : calls){
          //Note: already made sure it's a direct call when populating calls
          Function *called = ci->getCalledFunction();
          if(called->getName().equals(ret_name) && !shouldInstrumentFunc(*called))
            mod = false;
        }
        if(mod) mod_rets.push_back(ri); 
      }

      //Go over the returns, see if a call matches the function it's returning
      //from. If so, see if the call is directly used by a branch. If so, change
      //the value to something "hard to glitch".
      vector<User *> to_mod;
      for(ReturnInst *ri : mod_rets) {
        StringRef ret_name = ri->getFunction()->getName();
        to_mod.push_back(ri);
        bool shouldReplace = true;
        for (CallInst *ci : calls) {
          if(ci->getCalledFunction()->getName().equals(ret_name)){
            for(User *U : ci->users()){
              //TODO: should verify that we are comparing against a constant ...
              if(isa<Instruction>(U) && !isa<CmpInst>(U)){
                shouldReplace = false;
                break;
              }
              to_mod.push_back(U);
            }
          } 
        }
        if(shouldReplace){
          //TODO: fix up this constant
          ConstantInt *glitch_resistant = ConstantInt::get(Type::getInt32Ty(M.getContext()), 0x55555555);
          for(User *U : to_mod) {
            U->dump();
            if(ReturnInst *ri = dyn_cast<ReturnInst>(U)){
              ri->setOperand(0, glitch_resistant);
            }
            else if(CmpInst *cmp = dyn_cast<CmpInst>(U)){
              int idx = 0;
              for(Use &U : cmp->operands()){
                Value *v = U.get();
                errs() << "CmpInst candidate to replace: ";
                v->dump();
                if(isa<ConstantInt>(v)){
                  cmp->setOperand(idx, glitch_resistant);
                  break;
                }
                idx++;
              } 
            }
          }
        }
        to_mod.clear();
      }
      //if(isFunctionSafeToModify(&F)) {
        //errs() << "\033[1;31m[GR/Timing]\033[0m Instrumenting: " << F.getName() << "!\n";

      M.dump();
      errs() << "\n\n\n";
      return edited;
    }

  };

  char RetProtectorPass::ID = 0;
  // pass arg, pass desc, cfg_only, analysis only
  static RegisterPass<RetProtectorPass> x("injectRetProtector",
                                            "Instrument the provided module by "
                                            "inserting call glitch detector after every return",
                                            false,
                                            false);

  // Pass loading stuff
  // To use, run: clang -Xclang -load -Xclang <your-pass>.so <other-args> ...

  // This function is of type PassManagerBuilder::ExtensionFn
  static void loadPass(const PassManagerBuilder &Builder, llvm::legacy::PassManagerBase &PM) {
    PM.add(new RetProtectorPass());
  }
  // These constructors add our pass to a list of global extensions.
  static RegisterStandardPasses clangtoolLoader_Ox(PassManagerBuilder::EP_OptimizerLast, loadPass);
  static RegisterStandardPasses clangtoolLoader_O0(PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);
}
