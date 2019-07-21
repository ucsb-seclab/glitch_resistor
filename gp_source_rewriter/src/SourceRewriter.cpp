/***
 * Our main source rewriter.
 *
 */
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetSelect.h"

#include "GlobalProgramInfo.h"
#include "RWTypeDetector.h"

using namespace clang::driver;
using namespace clang::tooling;
using namespace clang;
using namespace llvm;
using namespace GLitchPlease;

static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("");

static cl::OptionCategory GPOptions("gp-source-rewriter options");

cl::opt<bool> Verbose("verbose",
                      cl::desc("Print verbose information"),
                      cl::init(false),
                      cl::cat(GPOptions));

cl::opt<bool> AllInitializers("allinit",
                              cl::desc("Replace an enum only if none "
                                       "of the enum values have initializers."),
                              cl::init(false),
                              cl::cat(GPOptions));

template <typename T, typename V>
class GenericAction : public ASTFrontendAction {
public:
  GenericAction(V &I) : Info(I) {}

  virtual std::unique_ptr<ASTConsumer>
  CreateASTConsumer(CompilerInstance &Compiler, StringRef InFile) {
    return std::unique_ptr<ASTConsumer>(new T(Info, &Compiler.getASTContext()));
  }

private:
  V &Info;
};


template <typename T>
std::unique_ptr<FrontendActionFactory>
newFrontendActionFactory(GlobalProgramInfo &I) {
  class ArgFrontendActionFactory : public FrontendActionFactory {
  public:
    explicit ArgFrontendActionFactory(GlobalProgramInfo &I) : Info(I) {}

    FrontendAction *create() override { return new T(Info); }

  private:
    GlobalProgramInfo &Info;
  };

  return std::unique_ptr<FrontendActionFactory>(
    new ArgFrontendActionFactory(I));
}

int main(int argc, const char **argv) {

  sys::PrintStackTraceOnErrorSignal(argv[0]);

  // Initialize targets for clang module support.
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmPrinters();
  InitializeAllAsmParsers();

  // set up all the compilation options to clang
  CommonOptionsParser OptionsParser(argc, argv, GPOptions);
  tooling::CommandLineArguments args = OptionsParser.getSourcePathList();
  ClangTool Tool(OptionsParser.getCompilations(), args);


  GlobalProgramInfo Info;

  // 1. Detect the types that needs to be written.
  std::unique_ptr<ToolAction> TypeDetectorTool = newFrontendActionFactory<
  GenericAction<RWTypeDetectorConsumer, GlobalProgramInfo>>(Info);

  if (TypeDetectorTool)
    Tool.run(TypeDetectorTool.get());
  else
    llvm_unreachable("No action");

  // 2. Identify the rewrite strategy i.e., hamming distance stuff

  // 3. Rewrite the source files (mostly header files) into a different folder.

  return 0;
}