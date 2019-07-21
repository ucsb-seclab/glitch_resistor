//
// Created by machiry at the beginning of time.
//
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"
#include "clang/AST/Decl.h"

#include "PersistentSourceLoc.h"
#include "RWTypeDetector.h"
#include "GlobalProgramInfo.h"
#include "Utils.h"

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

  bool VisitTagDecl(TagDecl *tagDecl) {
    if(tagDecl->isEnum()) {
      PersistentSourceLoc enumPSL = PersistentSourceLoc::mkPSL(tagDecl, *Context);
      EnumDecl *ED = dyn_cast<EnumDecl>(tagDecl);
      std::set<EnumConstantDecl*> toReplace;
      toReplace.clear();
      bool needToReplace = false;
      bool hasAtLeastOneInit = false;
      for(auto em: ED->enumerators()) {
        EnumConstantDecl *enumMem = em;
        toReplace.insert(enumMem);
        if(needToReplace) {
          continue;
        }
        if(!AllInitializers) {
          needToReplace = (enumMem->getInitExpr() == nullptr);
        } else {
          hasAtLeastOneInit = (enumMem->getInitExpr() != nullptr) || hasAtLeastOneInit;
        }
      }

      if(AllInitializers) {
        needToReplace = !hasAtLeastOneInit;
      }

      if(needToReplace) {
        SourceWithName enumKey = GlobalProgramInfo::getSourceNameKey(enumPSL, tagDecl->getName());
        for (auto em: toReplace) {
          PersistentSourceLoc fieldPSL = PersistentSourceLoc::mkPSL(em, *Context);
          SourceWithName fldKey = GlobalProgramInfo::getSourceNameKey(fieldPSL, em->getName());
          Info.insertEnumFieldToChange(enumKey, fldKey);
        }
      }

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