#pragma once

#include <string>

struct RuntimeLiveSummary {
    int compile_exit_code = -1;
    int run_exit_code = -1;
    bool compile_invoked = false;
    bool run_invoked = false;
    int source_units = 0;
    std::string compiler_binary;
    std::string compiler_command;
};

std::string BuildRuntimeLiveSummaryText(const RuntimeLiveSummary& summary);
