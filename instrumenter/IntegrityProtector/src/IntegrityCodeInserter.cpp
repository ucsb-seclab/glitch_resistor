//
// Created by Aravind Machiry on 2019-08-04.
//

#include "IntegrityCodeInserter.h"

using namespace GLitchPlease;

bool IntegrityCodeInserter::protectData(Value *src, Value *srcInt) {
  assert(src && srcInt && "Data items to change cannot be NULL");
  if(GlobalVariable *srcGlob = dyn_cast<GlobalVariable>(src)) {
    if(GlobalVariable *srcIntGlob = dyn_cast<GlobalVariable>(srcInt)) {
      return replicateAndIntegrityProtect(srcGlob, srcIntGlob);
    }
  }

  llvm::errs() << "[-] Unable to know replacement strategry for the instructions:" << *src <<
                  " and " << *srcInt << "\n";
  assert(false && "Invalid data items requested to be replaced.");
  return false;
}

bool IntegrityCodeInserter::canReplicateUsingReplacement(Value *currValue) {
  return dyn_cast<GetElementPtrInst>(currValue) || dyn_cast<CastInst>(currValue) || dyn_cast<PHINode>(currValue);
}

bool IntegrityCodeInserter::replicateAndIntegrityProtect(Value *srcInstr, Value *srcIntInstr) {
  // get usages of srcInstr.
  for(auto U : srcInstr->users()) {
    // Can this instruction be replicated using our default method?
    if (canReplicateUsingReplacement(U)) {
      // next set up the insertion point to be right after the original instruction.
      Instruction *targetInstr = dyn_cast<Instruction>(U);
      auto targetInsertPoint = targetInstr->getIterator();
      targetInsertPoint++;
      IRBuilder<> builder(&(*targetInsertPoint));
      std::string newValueName = targetInstr->getValueName()->first().str() + "_gpint";
      // clone the original instruction.
      Instruction *newInstr = targetInstr->clone();
      // replace the uses in the cloned instruction with the new instruction.
      newInstr->replaceUsesOfWith(srcInstr, srcIntInstr);
      // insert the cloned instruction into its new position.
      builder.Insert(newInstr, newValueName);
      // start propagating the cloned instruction.
      this->replicateAndIntegrityProtect(targetInstr, newInstr);
      continue;
    }

    if (auto srcGEP = dyn_cast<GEPOperator>(U)) {
      // this a GEP operator i.e., an inline GEP instruction.
      // get the operands and the pointer.
      std::vector<Value*> arrIdxVals;
      arrIdxVals.clear();
      bool pointerOpFound = false;
      for(unsigned i=0; i< srcGEP->getNumOperands(); i++) {
        Value *currOp = srcGEP->getOperand(i);
        // Is this the src pointer operand? if yes, skip it.
        if(currOp != srcGEP->getPointerOperand()) {
          arrIdxVals.push_back(currOp);
        } else {
          pointerOpFound = true;
        }
      }

      if(!pointerOpFound) {
        llvm::errs() << "[-] Unable to handle GEP instruction:" << *srcGEP << "\n";
      }
      assert(pointerOpFound && "Expected a GEP instruction to find a pointer but did not find one.");
      Instruction *targetSrcIntr = nullptr;

      for(auto U : srcGEP->users()) {
        if (auto I = dyn_cast<Instruction>(U)) {
          targetSrcIntr = I;
          break;
        }
      }

      // next set up the insertion point to be right after the original instruction.
      auto targetInsertPoint = targetSrcIntr->getIterator();
      targetInsertPoint++;
      IRBuilder<> builder(&(*targetInsertPoint));

      std::string newValueName = "tmp_st_gpint";

      Value *newGEPInstr = builder.CreateGEP(srcIntInstr, arrIdxVals, newValueName);
      // now start using the new GEP instruction.
      this->replicateAndIntegrityProtect(srcGEP, newGEPInstr);
      continue;
    }

    // if this is used in a load instruction?
    if(auto srcLoad = dyn_cast<LoadInst>(U)) {
      // Okay, here we need to insert the integrity load
      assert(srcLoad->getPointerOperand() == srcInstr &&
             "Sanity that the load instruction is using the src operand");
      // get the function to call.
      Function *targetFunc = this->fetchHelper.getReadHelperFunction(srcLoad->getType());

      // next set up the insertion point to be right after the original instruction.
      auto targetInsertPoint = srcLoad->getIterator();
      targetInsertPoint++;
      IRBuilder<> builder(&(*targetInsertPoint));

      Value *srcIntBC = builder.CreatePointerCast(srcIntInstr,
                                                  Type::getInt8PtrTy(m.getContext()), "loadgpicast");
      Value *srcBC = builder.CreatePointerCast(srcInstr,
                                               Type::getInt8PtrTy(m.getContext()), "loadgpocast");

      // Insert a call to the integrity checking function.
      Value *arguments[] = {srcBC, srcIntBC};
      Value *newVal = builder.CreateCall(targetFunc, arguments, "ReadGPValue");

      // if we are loading a pointer? we need to convert into correct type
      if(srcLoad->getType()->isPointerTy()) {
        newVal = builder.CreatePointerCast(newVal, srcLoad->getType(), "gpConToOrPtr");
      }

      for(auto loadUse : srcLoad->users()) {
        loadUse->replaceUsesOfWith(srcLoad, newVal);
      }

      srcLoad->eraseFromParent();

      // NO need to propagate this information, because we replaced all the instructions
      // that use the load with the result of the integrity load call.
      continue;
    }

    // if this is used in a store instruction?
    if(auto srcStore = dyn_cast<StoreInst>(U)) {
      assert(srcStore->getPointerOperand() == srcInstr &&
             "Sanity that the store instruction is using the src operand");
      // get the value to store.
      Value *toStoreValue = srcStore->getValueOperand();
      // next set up the insertion point to be before the original instruction.
      auto targetInsertPoint = srcStore->getIterator();
      IRBuilder<> builder(&(*targetInsertPoint));
      // Are we storing a pointer? then convert into void*
      if(toStoreValue->getType()->isPointerTy()) {
        toStoreValue = builder.CreatePointerCast(toStoreValue,
                                                 Type::getInt8PtrTy(m.getContext()), "storegpocast");
      }

      // convert the pointers into void*
      Value *srcIntBC = builder.CreatePointerCast(srcIntInstr,
                                                  Type::getInt8PtrTy(m.getContext()), "storegpicast");
      Value *srcBC = builder.CreatePointerCast(srcInstr,
                                               Type::getInt8PtrTy(m.getContext()), "storegpocast");

      // Insert a call to the integrity checking write function.
      Value *arguments[] = {srcBC, srcIntBC, toStoreValue};

      Function *targetWriteFunction = this->fetchHelper.getWriteHelperFunction(toStoreValue->getType());

      builder.CreateCall(targetWriteFunction, arguments);
      // delete the original store instruction.
      srcStore->eraseFromParent();
      // Similar to the load, no need to propagate this information.
      continue;
    }

    if(Instruction *UI = dyn_cast<Instruction>(U)) {
      llvm::errs() << "[-] Do not know how to handle the instruction:" << *UI << "\n";
    } else {
      llvm::errs() << "[-] Not an instruction:" << *U << "\n";
    }

    llvm::errs() << "[-] Invalid instruction type detected:" << *U << "\n";
    assert(false && "Invalid instruction detection while trying to replace.");

  }
  return true;
}