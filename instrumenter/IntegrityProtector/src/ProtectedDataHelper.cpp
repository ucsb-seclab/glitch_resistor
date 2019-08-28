//
// Created by Aravind Machiry on 2019-08-04.
//

#include "ProtectedDataHelper.h"

using namespace GLitchPlease;

#define GPINTEGRITYSECTIONNAME "gpprotect.integrity"

std::set<Value *> &ProtectedDataHandler::getToProtectDataElements() {
  if (toProtectDataVars.empty()) {
    for (auto &globV : m.getGlobalList()) {
      if (globV.hasName()) {
        std::string globName = globV.getName().str();
        // check if this variable needs to be protected?
        if (toProtectGlobVarNames.find(globName) !=
            toProtectGlobVarNames.end()) {
          // check if this does not have its address taken
          if (!isAddressTaken(&globV)) {
            // if (Verbose) {
            llvm::errs() << "[+] Global Variable: " << globName
                         << " is selected to be protected.\n";
            // }
            toProtectDataVars.insert(&globV);
          } else {
            // requested a variable to be protected, but the variable
            // has its address taken
            llvm::errs()
                << "[-] Global Variable: " << globName
                << " cannot be protected as its address has been taken.\n";
          }
        }
      }
    }
  }
  return toProtectDataVars;
}

std::map<Value *, Value *> &ProtectedDataHandler::getShadowDataMap() {
  getToProtectDataElements();
  if (shadowDataVars.empty() && !toProtectDataVars.empty()) {
    // ok, we need to create new shadow data vars.
    for (auto currDVar : toProtectDataVars) {
      GlobalVariable *gVar = dyn_cast<GlobalVariable>(currDVar);
      if (gVar) {
        std::string newGVarName = gVar->getName().str() + "_gpintegrity";
        Constant *newIntegrityVal = m.getOrInsertGlobal(
            newGVarName, gVar->getType()->getPointerElementType());
        assert(newIntegrityVal && "Failed to insert a new global variable.");
        // if(Verbose) {
        llvm::errs() << "[+] Created a new global variable:" << newGVarName
                     << " to protect:" << gVar->getName().str() << "\n";
        // }

        GlobalVariable *newGlobVar = dyn_cast<GlobalVariable>(newIntegrityVal);
        // set the linkage
        newGlobVar->setLinkage(gVar->getLinkage());
        // initialize this..only if we have initialize for
        // the source global variable.
        if (gVar->hasInitializer()) {
          // set the section name.
          newGlobVar->setSection(GPINTEGRITYSECTIONNAME);
          // initialize it to zeros.
          Constant *globInit = getConstantZeroForType(gVar->getType()->getPointerElementType());
          newGlobVar->setInitializer(globInit);
        }
        shadowDataVars[currDVar] = newIntegrityVal;
      } else {
        llvm::errs() << "[-] Cannot handle non-global value:" << *currDVar
                     << "\n";
      }
    }
  }
  return shadowDataVars;
}

bool ProtectedDataHandler::isAddressTaken(Value *toCheckValue) {
  // We only know how to check address taken for global variables.
  GlobalVariable *globVar = dyn_cast<GlobalVariable>(toCheckValue);
  // TODO: implement address taken detection mechanism.
  return globVar == nullptr;
}

Constant* ProtectedDataHandler::getConstantZeroForType(Type *targetType) {
  Constant *toRet = nullptr;
  if(targetType->isStructTy() || targetType->isArrayTy() || targetType->isVectorTy()) {
    toRet = ConstantAggregateZero::get(targetType);
  } else if(targetType->isIntegerTy()) {
    toRet = ConstantInt::get(targetType, 0);
  } else if(targetType->isFloatingPointTy()) {
    toRet = ConstantFP::get(targetType, 0.0);
  } else {
    assert(false && "Unable to handle the global type");
  }
  return toRet;
}