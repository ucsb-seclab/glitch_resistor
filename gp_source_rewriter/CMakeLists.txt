set( LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Option
  Support
  )

add_clang_executable(gp-source-rewriter
  src/SourceRewriter.cpp
  src/GlobalProgramInfo.cpp
  src/RWTypeDetector.cpp
  src/PersistentSourceLoc.cpp
  src/Utils.cpp
  )

include_directories(include/)

target_link_libraries(gp-source-rewriter
  PRIVATE
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangRewriteFrontend
  clangStaticAnalyzerFrontend
  clangTooling
  )

install(TARGETS gp-source-rewriter
  RUNTIME DESTINATION bin)
