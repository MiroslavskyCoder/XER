# ============================================================
# Required system / compiler dependencies
# ============================================================

find_package(absl REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# ── libnode / V8 ────────────────────────────────────────────
set(NODE_INCLUDE_DIR)
set(NODE_LIBRARY)
if(ENABLE_V8)
    find_path(NODE_INCLUDE_DIR
        NAMES v8.h
        PATH_SUFFIXES node
    )
    find_library(NODE_LIBRARY NAMES node)

    if(NOT NODE_INCLUDE_DIR OR NOT NODE_LIBRARY)
        message(FATAL_ERROR
            "libnode-dev not found: install package libnode-dev (v8.h + libnode), or configure with ENABLE_V8=OFF")
    endif()
else()
    message(STATUS "ENABLE_V8=OFF: skipping libnode/V8 dependency discovery")
endif()

# ── LLVM component lib list ──────────────────────────────────
llvm_map_components_to_libnames(LLVM_ALL_COMPONENT_LIBS all)

# ── Clang available libs ─────────────────────────────────────
set(CLANG_ALL_TARGETS
    clang
    clang-cpp
    clangAPINotes
    clangARCMigrate
    clangAST
    clangASTMatchers
    clangAnalysis
    clangAnalysisFlowSensitive
    clangAnalysisFlowSensitiveModels
    clangApplyReplacements
    clangBasic
    clangChangeNamespace
    clangCodeGen
    clangCrossTU
    clangDaemon
    clangDaemonTweaks
    clangDependencyScanning
    clangDirectoryWatcher
    clangDoc
    clangDriver
    clangDynamicASTMatchers
    clangEdit
    clangExtractAPI
    clangFormat
    clangFrontend
    clangFrontendTool
    clangHandleCXX
    clangHandleLLVM
    clangIncludeCleaner
    clangIncludeFixer
    clangIncludeFixerPlugin
    clangIndex
    clangInterpreter
    clangLex
    clangMove
    clangParse
    clangPseudo
    clangPseudoCLI
    clangPseudoCXX
    clangPseudoGrammar
    clangPseudoMatchers
    clangPseudoTest
    clangReorderFields
    clangRewrite
    clangRewriteFrontend
    clangSema
    clangSerialization
    clangStaticAnalyzerCheckers
    clangStaticAnalyzerCore
    clangStaticAnalyzerFrontend
    clangSupport
    clangTidy
    clangTidyAbseilModule
    clangTidyAlteraModule
    clangTidyAndroidModule
    clangTidyBoostModule
    clangTidyBugproneModule
    clangTidyCERTModule
    clangTidyConcurrencyModule
    clangTidyCppCoreGuidelinesModule
    clangTidyDarwinModule
    clangTidyFuchsiaModule
    clangTidyGoogleModule
    clangTidyHICPPModule
    clangTidyLLVMLibcModule
    clangTidyLLVMModule
    clangTidyLinuxKernelModule
    clangTidyMPIModule
    clangTidyMain
    clangTidyMiscModule
    clangTidyModernizeModule
    clangTidyObjCModule
    clangTidyOpenMPModule
    clangTidyPerformanceModule
    clangTidyPlugin
    clangTidyPortabilityModule
    clangTidyReadabilityModule
    clangTidyUtils
    clangTidyZirconModule
    clangTooling
    clangToolingASTDiff
    clangToolingCore
    clangToolingInclusions
    clangToolingInclusionsStdlib
    clangToolingRefactoring
    clangToolingSyntax
    clangTransformer
)

set(CLANG_AVAILABLE_LIBS)
foreach(clang_target IN LISTS CLANG_ALL_TARGETS)
    if(TARGET ${clang_target})
        list(APPEND CLANG_AVAILABLE_LIBS ${clang_target})
    endif()
endforeach()
