//
// Created by machiry on 7/21/19.
//

#ifndef LLVM_DEFENSEHELPER_H
#define LLVM_DEFENSEHELPER_H

#include "GlobalProgramInfo.h"

namespace GLitchPlease {
  /**
   * Class that holds all the logic to generate constansts.
   */
  class DefenseHelper {
  public:
    DefenseHelper(GlobalProgramInfo &I): Info(I) { }
    /**
     * Generate hamming distance constants for the
     * observed enum fields.
     */
    bool generateHammingConstantsForEnumFields();
  private:
    GlobalProgramInfo &Info;
  };
}

#endif //LLVM_DEFENSEHELPER_H
