#include "runtime_live_summary.h"

#include <sstream>

std::string BuildRuntimeLiveSummaryText(const RuntimeLiveSummary& summary) {
    std::ostringstream out;
    out << "\n=== RuntimeLive Summary ===\n";
    out << "compile invoked: " << (summary.compile_invoked ? "yes" : "no") << '\n';
    out << "run invoked: " << (summary.run_invoked ? "yes" : "no") << '\n';
    out << "compile exit: " << summary.compile_exit_code << '\n';
    out << "run exit: " << summary.run_exit_code << '\n';
    out << "source units: " << summary.source_units << '\n';

    if (!summary.compiler_binary.empty()) {
        out << "compiler: " << summary.compiler_binary << '\n';
    }
    if (!summary.compiler_command.empty()) {
        out << "command: " << summary.compiler_command << '\n';
    }

    out << "===========================\n";
    return out.str();
}
