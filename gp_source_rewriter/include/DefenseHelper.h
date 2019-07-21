//
// Created by machiry on 7/21/19.
//

#ifndef LLVM_DEFENSEHELPER_H
#define LLVM_DEFENSEHELPER_H

#include "GlobalProgramInfo.h"

namespace GLitchPlease {
  class DefenseHelper {
  public:
    DefenseHelper(GlobalProgramInfo &I): Info(I) { }
    bool generateHammingConstantsForEnumFields();
  private:
    GlobalProgramInfo &Info;
  };
}

#endif //LLVM_DEFENSEHELPER_H
