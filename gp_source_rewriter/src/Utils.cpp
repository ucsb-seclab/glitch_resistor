//
// Created by machiry on 7/20/19.
//

#include <llvm/Support/FileSystem.h>
#include "Utils.h"
#include "llvm/Support/Path.h"

using namespace llvm;

bool getAbsoluteFilePath(std::string fileName, std::string &absoluteFP) {
  // get absolute path of the provided file
  // returns true if successful else false
  SmallString<255> abs_path(fileName);
  std::error_code ec = llvm::sys::fs::make_absolute(abs_path);
  if(!ec) {
    absoluteFP = abs_path.str();
    return true;
  }
  return false;
}