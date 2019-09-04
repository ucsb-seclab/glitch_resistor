//
// Created by machiry at the beginning of time.
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
#include <set>
#include <vector>

using namespace llvm;
using namespace std;

namespace GLitchPlease {

static cl::OptionCategory GPOptions("retprotector options");

/***
 * The main pass.
 */
struct RetProtectorPass : public ModulePass {
public:
  static char ID;
  Function *delayFunction;
  std::set<Function *> annotFuncs;
  std::string AnnotationString = "NoResistor";
  std::string TAG = "\033[1;31m[GR/Ret]\033[0m ";
  bool Verbose = false;
  RetProtectorPass() : ModulePass(ID) { this->delayFunction = nullptr; }

  ~RetProtectorPass() {}

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
   * Get a list of all of the annotated functions
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
            if (Verbose) {
              dbgs() << "Found annotated function " << FUNC->getName() << "\n";
            }
          }
        }
      }
    }
  }

  bool runOnModule(Module &M) override {
    // Place a call to our detection function after the return of every function
    bool edited = false;

    // TODO: other bit sizes as well, but 32 seems to be the most common
    ConstantInt *zero = ConstantInt::get(Type::getInt32Ty(M.getContext()), 0);

    // TODO: Will likely need to handle invokes as well
    vector<ReturnInst *> rets;
    vector<CallInst *> calls;

    // Find all returns of a constant 0
    for (User *U : zero->users()) {
      Instruction *inst = dyn_cast<Instruction>(U);
      if (!inst || !shouldInstrumentFunc(*(inst->getFunction())))
        continue;
      else if (ReturnInst *ri = dyn_cast<ReturnInst>(inst)) {
        // check if address taken
        // ri->dump();
        // const User *zed;
        // errs() << "Address Taken: " <<
        // ri->getFunction()->hasAddressTaken(&zed) << "\n"; const
        // BitCastOperator *tmp = dyn_cast<BitCastOperator>(zed); if(tmp) {
        //  tmp->dump();
        //  for(const User *U2 : tmp->users())
        //    U2->dump();
        //}
        // if(!ri->getFunction()->hasAddressTaken())
        if (Verbose)
          errs() << TAG << "Found " << *ri << "\n";
        rets.push_back(ri);
      }
    }

    // Find All call instructions in the Module
    for (auto it = M.begin(); it != M.end(); it++) {
      Function &F = *it;
      for (BasicBlock &BB : F) {
        for (Instruction &I : BB) {
          Instruction *inst = &I;
          if (CallInst *ci = dyn_cast<CallInst>(inst)) {
            if (ci->getCalledFunction())
              calls.push_back(ci);
          }
        }
      }
    }

    // For each return instruction, verify that 1) it is in a modifiable
    // function and 2) *every* call to it is in a modifiable function.
    vector<ReturnInst *> mod_rets;
    for (ReturnInst *ri : rets) {
      bool mod = true;
      StringRef ret_name = ri->getFunction()->getName();
      for (CallInst *ci : calls) {
        // Note: already made sure it's a direct call when populating calls
        Function *called = ci->getCalledFunction();
        if (called->getName().equals(ret_name) &&
            !shouldInstrumentFunc(*called))
          mod = false;
      }
      if (mod)
        mod_rets.push_back(ri);
    }

    // Go over the returns, see if a call matches the function it's returning
    // from. If so, see if the call is directly used by a branch. If so, change
    // the value to something "hard to glitch".
    vector<User *> to_mod;
    for (ReturnInst *ri : mod_rets) {
      StringRef ret_name = ri->getFunction()->getName();
      to_mod.push_back(ri);
      bool shouldReplace = true;
      for (CallInst *ci : calls) {
        if (ci->getCalledFunction()->getName().equals(ret_name)) {
          for (User *U : ci->users()) {
            // TODO: should verify that we are comparing against a constant ...
            if (isa<Instruction>(U) && !isa<CmpInst>(U)) {
              shouldReplace = false;
              break;
            }
            to_mod.push_back(U);
          }
        }
      }
      if (shouldReplace) {
        errs() << TAG << "Modified return address for: "
               << ri->getFunction()->getName() << "\n";
        // TODO: fix up this constant
        ConstantInt *glitch_resistant =
            ConstantInt::get(Type::getInt32Ty(M.getContext()), 0x55555555);
        for (User *U : to_mod) {
          if (ReturnInst *ri = dyn_cast<ReturnInst>(U)) {
            ri->setOperand(0, glitch_resistant);
          } else if (CmpInst *cmp = dyn_cast<CmpInst>(U)) {
            int idx = 0;
            for (Use &U : cmp->operands()) {
              Value *v = U.get();
              if (isa<ConstantInt>(v) && v == zero) {
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

    // M.dump();
    return edited;
  }
};

char RetProtectorPass::ID = 0;
// pass arg, pass desc, cfg_only, analysis only
static RegisterPass<RetProtectorPass>
    x("injectRetProtector",
      "Instrument the provided module by "
      "inserting call glitch detector after every return",
      false, false);

// Pass loading stuff
// To use, run: clang -Xclang -load -Xclang <your-pass>.so <other-args> ...

// This function is of type PassManagerBuilder::ExtensionFn
static void loadPass(const PassManagerBuilder &Builder,
                     llvm::legacy::PassManagerBase &PM) {
  PM.add(new RetProtectorPass());
}
// These constructors add our pass to a list of global extensions.
static RegisterStandardPasses
    clangtoolLoader_Ox(PassManagerBuilder::EP_OptimizerLast, loadPass);
static RegisterStandardPasses
    clangtoolLoader_O0(PassManagerBuilder::EP_EnabledOnOptLevel0, loadPass);
} // namespace GLitchPlease
