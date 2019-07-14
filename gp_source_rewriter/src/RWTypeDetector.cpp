//
// Created by machiry at the beginning of time.
//
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"

#include "RWTypeDetector.h"

using namespace GLitchPlease;
using namespace clang;
using namespace llvm;

/***
 * This is an AST Visitor that identifies the type to rewrite.
 */
class RWTypeDetectorVisitor : public RecursiveASTVisitor<RWTypeDetectorVisitor> {
public:
  explicit RWTypeDetectorVisitor(ASTContext *Context, GlobalProgramInfo &I)
    : Context(Context), Info(I) {}

  bool VisitTagDecl(TagDecl *Decl) {
    if(Decl->isEnum()) {
      dbgs() << "enum\n";
      Decl->dump();
    }
    return true;
  }

private:
  ASTContext *Context;
  GlobalProgramInfo &Info;
};

void RWTypeDetectorConsumer::HandleTranslationUnit(ASTContext &C) {
  // here, we are inside one translation unit. i.e., one C file.
  RWTypeDetectorVisitor GV = RWTypeDetectorVisitor(&C, Info);
  TranslationUnitDecl *TUD = C.getTranslationUnitDecl();
  // Try to see if any of the declarations is a type that
  // we would like to rewrite.
  for (const auto &D : TUD->decls()) {
    GV.TraverseDecl(D);
  }
  return;
}