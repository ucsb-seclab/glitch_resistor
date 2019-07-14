//
// Created by machiry at the beginning of time
//

#ifndef LLVM_GLOBALPROGRAMINFO_H
#define LLVM_GLOBALPROGRAMINFO_H

/***
 * In clang, we get one compilation unit (i.e., one C file) at a time. To perform any global analysis,
 * we should store information about each of the compilation unit in this global program info structure.
 * Perform the analysis and then rewrite the content back to the source files.
 *
 * Without this common structure, there is no way we can ensure the we consistently rewritten all the files.
 */


namespace GLitchPlease {
  /***
   * Class that contains information about all the C files that are being analyzed.
   */
  class GlobalProgramInfo {

  };
}

#endif //LLVM_GLOBALPROGRAMINFO_H
