//
// Created by machiry at the beginning of time.
//

#ifndef LLVM_RWTYPEDETECTOR_H
#define LLVM_RWTYPEDETECTOR_H

#include "GlobalProgramInfo.h"

namespace GLitchPlease {
  /***
   * Class that detects the type definitions that need to be rewritten.
   */
  class RWTypeDetectorConsumer: public clang::ASTConsumer {
  public:
    explicit RWTypeDetectorConsumer(GlobalProgramInfo &I, clang::ASTContext *C) :
      Info(I) { }

    virtual void HandleTranslationUnit(clang::ASTContext &);

  private:
    GlobalProgramInfo &Info;
  };
}

#endif //LLVM_RWTYPEDETECTOR_H
