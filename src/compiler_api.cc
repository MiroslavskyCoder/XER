#include "compiler_api.h"
#include "tool_to.h"

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticIDs.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>

#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/raw_ostream.h>

#include <array>
#include <optional>
#include <utility>

bool CompilerApi::Compile(const std::string& compiler_binary,
                          const std::vector<std::string>& args,
                          const std::filesystem::path& stdout_path,
                          const std::filesystem::path& stderr_path,
                          int* exit_code) const {
    llvm::ErrorOr<std::string> resolved_program = llvm::sys::findProgramByName(compiler_binary);
    if (!resolved_program) {
        return false;
    }

    return CompileInProcess(*resolved_program, args, stdout_path, stderr_path, exit_code);
}

bool CompilerApi::RunBinary(const std::filesystem::path& binary_path,
                            const std::filesystem::path& stdout_path,
                            const std::filesystem::path& stderr_path,
                            int* exit_code) const {
    return ExecuteProcess(binary_path.string(), {}, stdout_path, stderr_path, exit_code, false);
}

bool CompilerApi::CompileInProcess(const std::string& compiler_binary,
                                   const std::vector<std::string>& args,
                                   const std::filesystem::path& stdout_path,
                                   const std::filesystem::path& stderr_path,
                                   int* exit_code) const {
    {
        std::string io_error;
        const bool out_ok = ToolTo::WriteTextFile(stdout_path, std::string(), false, &io_error);
        const bool err_ok = ToolTo::WriteTextFile(stderr_path, std::string(), false, &io_error);
        if (!out_ok || !err_ok) {
            return false;
        }
    }

    std::string diagnostics_text;
    llvm::raw_string_ostream diagnostics_stream(diagnostics_text);

    auto diag_ids = llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs>(new clang::DiagnosticIDs());
    auto diag_options = llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions>(new clang::DiagnosticOptions());
    auto diag_printer = std::make_unique<clang::TextDiagnosticPrinter>(diagnostics_stream, &*diag_options);

    clang::DiagnosticsEngine diagnostics_engine(
        diag_ids,
        &*diag_options,
        diag_printer.release(),
        true);

    clang::driver::Driver driver(compiler_binary,
                                 llvm::sys::getDefaultTargetTriple(),
                                 diagnostics_engine);
    driver.setCheckInputsExist(false);

    std::vector<std::string> argv_storage;
    argv_storage.reserve(args.size() + 1);
    argv_storage.push_back(compiler_binary);
    for (const std::string& arg : args) {
        argv_storage.push_back(arg);
    }

    std::vector<const char*> argv;
    argv.reserve(argv_storage.size());
    for (const std::string& token : argv_storage) {
        argv.push_back(token.c_str());
    }

    std::unique_ptr<clang::driver::Compilation> compilation(driver.BuildCompilation(argv));
    int local_exit_code = 1;

    if (compilation != nullptr) {
        llvm::SmallVector<std::pair<int, const clang::driver::Command*>, 4> failing_commands;
        local_exit_code = driver.ExecuteCompilation(*compilation, failing_commands);

        if (!failing_commands.empty()) {
            if (local_exit_code == 0) {
                local_exit_code = 1;
            }
            diagnostics_stream << "Failing commands:\n";
            for (const auto& failed : failing_commands) {
                if (failed.second != nullptr) {
                    diagnostics_stream << "  (" << failed.first << ") "
                                       << failed.second->getExecutable() << "\n";
                }
            }
        }
    } else {
        diagnostics_stream << "Unable to build clang compilation pipeline.\n";
    }

    diagnostics_stream.flush();

    std::string io_error;
    if (!ToolTo::WriteTextFile(stderr_path, diagnostics_text, false, &io_error)) {
        return false;
    }

    if (exit_code != nullptr) {
        *exit_code = local_exit_code;
    }

    return true;
}
