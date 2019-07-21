//
// Created by machiry at the beginning of time
//

#ifndef LLVM_GLOBALPROGRAMINFO_H
#define LLVM_GLOBALPROGRAMINFO_H

#include "PersistentSourceLoc.h"

/***
 * In clang, we get one compilation unit (i.e., one C file) at a time. To perform any global analysis,
 * we should store information about each of the compilation unit in this global program info structure.
 * Perform the analysis and then rewrite the content back to the source files.
 *
 * Without this common structure, there is no way we can ensure the we consistently rewritten all the files.
 */


namespace GLitchPlease {

  // pair the represents source location and name of source element.
  typedef std::pair<PersistentSourceLoc, std::string> SourceWithName;
  // clang independent storage of enums to be replaced.
  typedef std::map<SourceWithName, std::set<SourceWithName>> EnumFields;
  typedef std::map<SourceWithName, unsigned> FieldConstMap;

  /***
   * Class that contains information about all the C files that are being analyzed.
   */
  class GlobalProgramInfo {
  public:
    GlobalProgramInfo();
    virtual ~GlobalProgramInfo() { };

    static SourceWithName getSourceNameKey(PersistentSourceLoc &psl, std::string sname);

    bool insertEnumFieldToChange(SourceWithName &enumKey, SourceWithName &fieldKey);

    EnumFields& getToChangeEnumFields() { return toChangeEnumFields; };
    bool insertFieldConstant(SourceWithName &fldKey, unsigned newConst) {
      generatedConstants[fldKey] = newConst;
    }

  private:
    EnumFields toChangeEnumFields;
    FieldConstMap generatedConstants;
  };
}

#endif //LLVM_GLOBALPROGRAMINFO_H
