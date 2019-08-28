//
// Created by machiry on 7/21/19.
//

#include "llvm/Support/raw_ostream.h"
#include <string>

#include "RewriteHelper.h"
#include "Utils.h"

using namespace clang;
using namespace llvm;
using namespace GLitchPlease;

std::string TAG = "\033[1;31m[GR/Enum]\033[0m ";

/**
 *  Check if we can rewrite the provided source range.
 * @param R Rewriter to use.
 * @param SR Source range to write.
 * @return true if can be rewritten else false.
 */
static bool canRewrite(Rewriter &R, SourceRange &SR) {
  return SR.isValid() && (R.getRangeSize(SR) != -1);
}

/***
 *  This class detects the enums in the current compilation unit.
 */
class EnumDetector : public RecursiveASTVisitor<EnumDetector> {
public:
  explicit EnumDetector(ASTContext *Context, GlobalProgramInfo &I)
      : Context(Context), Info(I) {
    foundEnums.clear();
    foundEnumFields.clear();
  }

  bool VisitTagDecl(TagDecl *tagDecl) {
    if (tagDecl->isEnum()) {
      // is this enum?
      PersistentSourceLoc enumPSL =
          PersistentSourceLoc::mkPSL(tagDecl, *Context);
      SourceWithName enumKey =
          GlobalProgramInfo::getSourceNameKey(enumPSL, tagDecl->getName());
      foundEnums.insert(enumKey);
      EnumDecl *ED = dyn_cast<EnumDecl>(tagDecl);
      for (auto em : ED->enumerators()) {
        // get all the fields and store them.
        PersistentSourceLoc fieldPSL = PersistentSourceLoc::mkPSL(em, *Context);
        SourceWithName fldKey =
            GlobalProgramInfo::getSourceNameKey(fieldPSL, em->getName());
        foundEnumFields[fldKey] = em;
      }
    }
    return true;
  }

  std::set<SourceWithName> &getFoundEnums() { return foundEnums; }

  std::map<SourceWithName, EnumConstantDecl *> &getFoundEnumFields() {
    return foundEnumFields;
  }

private:
  std::set<SourceWithName> foundEnums;
  std::map<SourceWithName, EnumConstantDecl *> foundEnumFields;
  ASTContext *Context;
  GlobalProgramInfo &Info;
};

/***
 *  Check if we can rewrite the file at the given file path.
 *  The check is based on the base-directory.
 *  All the files inside the base-directory are re-writable.
 *
 *  This is to avoid rewriting enums defined in default header files i.e.,
 * stdio.h
 * @param filePath file path of the file to be rewritten.
 * @param iof  input files provided on the command line.
 * @param b base-directory.
 * @return true if this can be rewritten else false.
 */
static bool canWrite(std::string filePath, std::set<std::string> &iof,
                     std::string b) {
  // Was this file explicitly provided on the command line?
  if (iof.count(filePath) > 0)
    return true;
  // Is this file contained within the base directory?
  // TODO: optimize this function.

  sys::path::const_iterator baseIt = sys::path::begin(b);
  sys::path::const_iterator pathIt = sys::path::begin(filePath);
  sys::path::const_iterator baseEnd = sys::path::end(b);
  sys::path::const_iterator pathEnd = sys::path::end(filePath);
  std::string baseSoFar = (*baseIt).str() + sys::path::get_separator().str();
  std::string pathSoFar = (*pathIt).str() + sys::path::get_separator().str();
  ++baseIt;
  ++pathIt;

  while ((baseIt != baseEnd) && (pathIt != pathEnd)) {
    sys::fs::file_status baseStatus;
    sys::fs::file_status pathStatus;
    std::string s1 = (*baseIt).str();
    std::string s2 = (*pathIt).str();

    if (std::error_code ec = sys::fs::status(baseSoFar, baseStatus))
      return false;

    if (std::error_code ec = sys::fs::status(pathSoFar, pathStatus))
      return false;

    if (!sys::fs::equivalent(baseStatus, pathStatus))
      break;

    if (s1 != sys::path::get_separator().str())
      baseSoFar += (s1 + sys::path::get_separator().str());
    if (s2 != sys::path::get_separator().str())
      pathSoFar += (s2 + sys::path::get_separator().str());

    ++baseIt;
    ++pathIt;
  }

  if (baseIt == baseEnd && baseSoFar == pathSoFar)
    return true;
  else
    return false;
}

/**
 *  This is the method the actually writes the output buffer corresponding
 *  to the provided file ids.
 * @param R clang Rewriter.
 * @param C ASTContext
 * @param Files set of file ids whose buffer should be written to files.
 * @param InOutFiles set of files provided on the command line.
 * @param BaseDir base directory of the sources we are converting.
 * @param OutputPostfix the postfix string that should be used to write files
 * out.
 */
static void emit(Rewriter &R, ASTContext &C, std::set<FileID> &Files,
                 std::set<std::string> &InOutFiles, std::string &BaseDir,
                 std::string &OutputPostfix) {

  // Check if we are outputing to stdout or not, if we are, just output the
  // main file ID to stdout.
  if (Verbose)
    errs() << "Writing files out\n";

  SmallString<254> baseAbs(BaseDir);
  std::error_code ec = sys::fs::make_absolute(baseAbs);
  assert(!ec);
  sys::path::remove_filename(baseAbs);
  std::string base = baseAbs.str();

  SourceManager &SM = C.getSourceManager();
  if (OutputPostfix == "-") {
    if (const RewriteBuffer *B = R.getRewriteBufferFor(SM.getMainFileID()))
      B->write(outs());
  } else
    for (const auto &F : Files)
      if (const RewriteBuffer *B = R.getRewriteBufferFor(F))
        if (const FileEntry *FE = SM.getFileEntryForID(F)) {
          assert(FE->isValid());

          // Produce a path/file name for the rewritten source file.
          // That path should be the same as the old one, with a
          // suffix added between the file name and the extension.
          // For example \foo\bar\a.c should become \foo\bar\a.gplease.c
          // if the OutputPostfix parameter is "gplease" .

          std::string pfName = sys::path::filename(FE->getName()).str();
          std::string dirName = sys::path::parent_path(FE->getName()).str();
          std::string fileName =
              sys::path::remove_leading_dotslash(pfName).str();
          std::string ext = sys::path::extension(fileName).str();
          std::string stem = sys::path::stem(fileName).str();
          std::string nFileName = stem + "." + OutputPostfix + ext;
          std::string nFile = nFileName;
          if (dirName.size() > 0)
            nFile = dirName + sys::path::get_separator().str() + nFileName;

          // Write this file out if it was specified as a file on the command
          // line.
          SmallString<254> feAbs(FE->getName());
          std::string feAbsS = "";
          if (std::error_code ec = sys::fs::make_absolute(feAbs)) {
            if (Verbose)
              errs() << "could not make path absolote\n";
          } else
            feAbsS = sys::path::remove_leading_dotslash(feAbs.str());

          if (canWrite(feAbsS, InOutFiles, base)) {
            std::error_code EC;
            raw_fd_ostream out(nFile, EC, sys::fs::F_None);

            if (!EC) {
              if (Verbose)
                outs() << "writing out " << nFile << "\n";
              B->write(out);
            } else
              errs() << "could not open file " << nFile << "\n";
            // This is awkward. What to do? Since we're iterating,
            // we could have created other files successfully. Do we go back
            // and erase them? Is that surprising? For now, let's just keep
            // going.
          }
        }
}

void RewriteConsumer::HandleTranslationUnit(ASTContext &Context) {

  std::map<EnumConstantDecl *, std::string> rewriteContentMap;
  rewriteContentMap.clear();

  Rewriter R(Context.getSourceManager(), Context.getLangOpts());

  // 1. first get all the enum fields in this translation unit.
  EnumDetector ED = EnumDetector(&Context, Info);
  TranslationUnitDecl *TUD = Context.getTranslationUnitDecl();
  for (const auto &D : TUD->decls()) {
    ED.TraverseDecl(D);
  }

  auto &foundEnumFields = ED.getFoundEnumFields();

  // 2. check if any of the identified enums needs to be rewritten.
  for (auto &fldConstant : Info.getFieldConstantMap()) {
    if (foundEnumFields.find(fldConstant.first) != foundEnumFields.end()) {
      EnumConstantDecl *enumDecl = foundEnumFields[fldConstant.first];
      std::string valString = std::to_string(fldConstant.second);
      std::string toReplace = enumDecl->getNameAsString() + " = " + valString;
      rewriteContentMap[enumDecl] = toReplace;
    }
  }
  std::set<FileID> ToRewriteFiles;
  // rewrite-each of the enum fields.
  for (const auto &N : rewriteContentMap) {
    Decl *D = N.first;
    assert(D != nullptr);

    if (Verbose) {
      errs() << "Replacing type of decl:\n";
      D->dump();
      errs() << "with " << N.second << "\n";
    } else {
      errs() << TAG << "Replaced " << N.first->getNameAsString() << " with "
             << N.second << "\n";
    }

    // Get a FullSourceLoc for the start location and add it to the
    // list of file ID's we've touched.
    SourceRange tTR = D->getSourceRange();

    if (canRewrite(R, tTR)) {
      R.ReplaceText(tTR, N.second);
      FullSourceLoc tFSL(tTR.getBegin(), Context.getSourceManager());
      ToRewriteFiles.insert(tFSL.getFileID());
    }
  }
  // output the files.
  emit(R, Context, ToRewriteFiles, InOutFiles, BaseDir, OutputPostfix);
}