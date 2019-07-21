//
// Created by machiry on 7/21/19.
//

#ifndef LLVM_REWRITEHELPER_H
#define LLVM_REWRITEHELPER_H

#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/ASTContext.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include "GlobalProgramInfo.h"

using namespace clang;
namespace GLitchPlease {

  /**
   * This class handles the rewriting of the files.
   */
  class RewriteConsumer : public ASTConsumer {
  public:
    explicit RewriteConsumer(GlobalProgramInfo &I,
                             std::set<std::string> &F, ASTContext *Context,
                             std::string &OPostfix, std::string &bDir) :
      Info(I), InOutFiles(F), OutputPostfix(OPostfix), BaseDir(bDir) {}

    virtual void HandleTranslationUnit(ASTContext &Context);

  private:
    GlobalProgramInfo &Info;
    std::set<std::string> &InOutFiles;
    std::string &OutputPostfix;
    std::string &BaseDir;
  };

}
#endif //LLVM_REWRITEHELPER_H
