//
// Created by machiry on 7/21/19.
//

#include "DefenseHelper.h"
#include "Utils.h"

using namespace GLitchPlease;
using namespace llvm;

bool DefenseHelper::generateHammingConstantsForEnumFields() {
  EnumFields &toChange = Info.getToChangeEnumFields();
  for(auto &enumInfo: toChange) {
    if(Verbose) {
      errs() << "[*] Trying to generate new enum-constants for the enum:" << enumInfo.first.second << "\n";
    }

    // enumInfo.second is the set of enum fields
    // for which the constants need to be generated.
    std::vector<unsigned> enumConstants;
    enumConstants.clear();

    // generate numbers with maximum hamming distance.
    generateNumbersWithMaximumHamming(enumInfo.second.size(), enumConstants);

    
    unsigned toInsert = 0;
    for(auto fldKey: enumInfo.second) {
      Info.insertFieldConstant(fldKey, enumConstants[toInsert]);
      toInsert++;
    }
  }
  return true;
}