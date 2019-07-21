//
// Created by machiry at the beginning of time.
//

#include "GlobalProgramInfo.h"

using namespace GLitchPlease;

GlobalProgramInfo::GlobalProgramInfo() {
  toChangeEnumFields.clear();
}

SourceWithName GlobalProgramInfo::getSourceNameKey(PersistentSourceLoc &psl, std::string sname) {
  return std::make_pair(psl, sname);
}

bool GlobalProgramInfo::insertEnumFieldToChange(SourceWithName &enumKey, SourceWithName &fieldKey) {
  return toChangeEnumFields[enumKey].insert(fieldKey).second;
}