#pragma once

#include "compilerapi/compiler_api.h"
#include "compilerapi/compiler_source.h"

#include <string>
#include <vector>

class Compiler {
public:
    struct ExecutionReport {
        bool compile_invoked = false;
        bool run_invoked = false;
        int compile_exit_code = -1;
        int run_exit_code = -1;
        std::string compiler_binary;
        std::vector<std::string> compiler_args;
    };

    Compiler();

    bool CompileAndRun(const CompilerSource& source,
                       int* compile_exit_code,
                       int* run_exit_code) const;

    bool CompileAndRunDetailed(const CompilerSource& source,
                               ExecutionReport* report) const;

private:
    CompilerApi api_;
};
