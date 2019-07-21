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
#include "DefenseHelper.h"
#include "RewriteHelper.h"

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


static cl::opt<std::string>
  BaseDir("base-dir",
          cl::desc("Base directory for the code we're translating"),
          cl::init(""),
          cl::cat(GPOptions));

static cl::opt<std::string>
  OutputPostfix("output-postfix",
                cl::desc("Postfix to add to the names of rewritten files, if "
                         "not supplied writes to STDOUT"),
                cl::init("-"), cl::cat(GPOptions));

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

template <typename T, typename V, typename U>
class RewriteAction : public ASTFrontendAction {
public:
  RewriteAction(V &I, U &P) : Info(I),Files(P) {}

  virtual std::unique_ptr<ASTConsumer>
  CreateASTConsumer(CompilerInstance &Compiler, StringRef InFile) {
    return std::unique_ptr<ASTConsumer>
      (new T(Info, Files, &Compiler.getASTContext(), OutputPostfix, BaseDir));
  }

private:
  V &Info;
  U &Files;
};

template <typename T>
std::unique_ptr<FrontendActionFactory>
newFrontendActionFactoryB(GlobalProgramInfo &I, std::set<std::string> &PS) {
  class ArgFrontendActionFactory : public FrontendActionFactory {
  public:
    explicit ArgFrontendActionFactory(GlobalProgramInfo &I,
                                      std::set<std::string> &PS) : Info(I),Files(PS) {}

    FrontendAction *create() override { return new T(Info, Files); }

  private:
    GlobalProgramInfo &Info;
    std::set<std::string> &Files;
  };

  return std::unique_ptr<FrontendActionFactory>(
    new ArgFrontendActionFactory(I, PS));
}

int main(int argc, const char **argv) {

  sys::PrintStackTraceOnErrorSignal(argv[0]);

  // Initialize targets for clang module support.
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllAsmPrinters();
  InitializeAllAsmParsers();

  if (BaseDir.size() == 0) {
    SmallString<256>  cp;
    if (std::error_code ec = sys::fs::current_path(cp)) {
      errs() << "could not get current working dir\n";
      return 1;
    }

    BaseDir = cp.str();
  }

  // set up all the compilation options to clang
  CommonOptionsParser OptionsParser(argc, argv, GPOptions);
  tooling::CommandLineArguments args = OptionsParser.getSourcePathList();
  ClangTool Tool(OptionsParser.getCompilations(), args);

  std::set<std::string> inoutPaths;

  for (const auto &S : args) {
    SmallString<255> abs_path(S);
    if (std::error_code ec = sys::fs::make_absolute(abs_path))
      errs() << "could not make absolute\n";
    else
      inoutPaths.insert(abs_path.str());
  }

  if (OutputPostfix == "-" && inoutPaths.size() > 1) {
    errs() << "If rewriting more than one , can't output to stdout\n";
    return 1;
  }


  GlobalProgramInfo Info;

  // 1. Detect the types that needs to be written.
  errs() << "[*] Trying to detect enum fields.\n";
  std::unique_ptr<ToolAction> TypeDetectorTool = newFrontendActionFactory<
  GenericAction<RWTypeDetectorConsumer, GlobalProgramInfo>>(Info);

  if (TypeDetectorTool)
    Tool.run(TypeDetectorTool.get());
  else
    llvm_unreachable("No action");

  errs() << "[*] Detecting hamming constants for enums\n";
  // 2. Identify the rewrite strategy i.e., hamming distance stuff
  DefenseHelper DH = DefenseHelper(Info);
  DH.generateHammingConstantsForEnumFields();

  // 3. Rewrite the source files (mostly header files) into a different folder.
  errs() << "[*] Trying to rewrite files\n";
  std::unique_ptr<ToolAction> RewriteTool =
    newFrontendActionFactoryB
      <RewriteAction<RewriteConsumer, GlobalProgramInfo, std::set<std::string>>>(Info, inoutPaths);

  if (RewriteTool)
    Tool.run(RewriteTool.get());
  else
    llvm_unreachable("No action");

  return 0;
}