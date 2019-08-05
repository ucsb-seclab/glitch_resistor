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

bool IntegrityCodeInserter::replicateAndIntegrityProtect(Value *srcInstr, Value *srcIntInstr) {
  // get usages of srcInstr.
  for(auto U : srcInstr->users()){
    // if it is used in a GetElementPtr?..then insert similar getelementptr instruction
    // and try to use the replicated instruction.
    if (auto srcGEP = dyn_cast<GetElementPtrInst>(U)){
      // first prepare operands to insert a new GEP instruction.
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

      // next set up the insertion point to be right after the original instruction.
      auto targetInsertPoint = srcGEP->getIterator();
      targetInsertPoint++;
      IRBuilder<> builder(&(*targetInsertPoint));

      std::string newValueName = srcGEP->getValueName()->first().str() + "_gpint";

      Value *newGEPInstr = builder.CreateGEP(srcGEP->getType(), srcIntInstr, arrIdxVals, newValueName);
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
                                                  Type::getInt8PtrTy(m.getContext()), srcLoad->getValueName()->first().str() + "_loadgpicast");
      Value *srcBC = builder.CreatePointerCast(srcInstr,
                                               Type::getInt8PtrTy(m.getContext()), srcLoad->getValueName()->first().str() + "_loadgpocast");

      // Insert a call to the integrity checking function.
      Value *arguments[] = {srcBC, srcIntBC};
      Value *newVal = builder.CreateCall(targetFunc, arguments, "ReadGPValue");

      // if we are loading a pointer? we need to convert into correct type
      if(srcLoad->getType()->isPointerTy()) {
        newVal = builder.CreatePointerCast(newVal, srcLoad->getType(), "gpConToOrPtr");
      }

      // replace the original instruction with the new function call.
      srcLoad->replaceUsesOfWith(srcLoad, newVal);
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
                                                 Type::getInt8PtrTy(m.getContext()), toStoreValue->getValueName()->first().str() + "_storegpocast");
      }

      // convert the pointers into void*
      Value *srcIntBC = builder.CreatePointerCast(srcIntInstr,
                                                  Type::getInt8PtrTy(m.getContext()), srcStore->getValueName()->first().str() + "_storegpicast");
      Value *srcBC = builder.CreatePointerCast(srcInstr,
                                               Type::getInt8PtrTy(m.getContext()), srcStore->getValueName()->first().str() + "_storegpocast");

      // Insert a call to the integrity checking write function.
      Value *arguments[] = {srcBC, srcIntBC, toStoreValue};

      Function *targetWriteFunction = this->fetchHelper.getWriteHelperFunction(toStoreValue->getType());

      builder.CreateCall(targetWriteFunction, arguments);
      // delete the original store instruction.
      srcStore->eraseFromParent();
      // Similar to the load, no need to propagate this information.
      continue;
    }

    llvm::errs() << "[-] Invalid instruction type detected:" << *U << "\n";
    assert(false && "Invalid instruction detection while trying to replace.");

  }
  return true;
}