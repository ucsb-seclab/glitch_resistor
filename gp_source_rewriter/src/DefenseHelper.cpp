//
// Created by machiry on 7/21/19.
//

#include "DefenseHelper.h"

using namespace GLitchPlease;

bool DefenseHelper::generateHammingConstantsForEnumFields() {
  EnumFields &toChange = Info.getToChangeEnumFields();
  for(auto &enumInfo: toChange) {
    // enumInfo.second is the set of enum fields
    // for which the constants need to be generated.
    // TODO: generate valid constants.
    unsigned toInsert = 0;
    for(auto fldKey: enumInfo.second) {
      Info.insertFieldConstant(fldKey, toInsert);
      toInsert++;
    }
  }
}