//
// Created by machiry on 7/21/19.
//

#include "RewriteHelper.h"

using namespace clang;

static bool canRewrite(Rewriter &R, SourceRange &SR) {
  return SR.isValid() && (R.getRangeSize(SR) != -1);
}
