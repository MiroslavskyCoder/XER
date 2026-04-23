#include "compiler.h" 

#include <utility>

Compiler::Compiler() = default;

bool Compiler::CompileAndRun(const CompilerSource& source,
                             int* compile_exit_code,
                             int* run_exit_code) const {
    ExecutionReport report;
    const bool ok = CompileAndRunDetailed(source, &report);
    if (compile_exit_code != nullptr) {
        *compile_exit_code = report.compile_exit_code;
    }
    if (run_exit_code != nullptr) {
        *run_exit_code = report.run_exit_code;
    }
    return ok;
}

bool Compiler::CompileAndRunDetailed(const CompilerSource& source,
                                     ExecutionReport* report) const {
    ExecutionReport local_report;
    ExecutionReport* out = report != nullptr ? report : &local_report;

    const std::vector<std::string> compiler_candidates = source.use_cpp()
                                                              ? std::vector<std::string>{"clang++", "c++"}
                                                              : std::vector<std::string>{"clang", "cc"};
    std::vector<std::string> args = source.BuildCompilerArgs();
    out->compiler_args = args;

    int compile_code = -1;
    for (const std::string& compiler_binary : compiler_candidates) {
        if (api_.Compile(compiler_binary,
                         args,
                         source.compile_out_path(),
                         source.compile_err_path(),
                         &compile_code)) {
            out->compile_invoked = true;
            out->compiler_binary = compiler_binary;
            break;
        }
    }

    out->compile_exit_code = compile_code;

    if (!out->compile_invoked) {
        return false;
    }

    if (compile_code != 0) {
        return true;
    }

    int run_code = -1;
    if (!api_.RunBinary(source.binary_path(),
                        source.run_out_path(),
                        source.run_err_path(),
                        &run_code)) {
        return false;
    }

    out->run_invoked = true;
    out->run_exit_code = run_code;

    return true;
}
