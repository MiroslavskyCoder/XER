#include "sandbox/sandbox_executor.h"
#include <array>
#include <cstdio>
#include <stdexcept>

namespace EngineDoctor {

SandboxExecutor::SandboxExecutor(const SandboxPolicy& policy)
    : policy_(policy) {}

ExecResult SandboxExecutor::Execute(const std::string& script_path) {
    ExecResult result;

    if (policy_.max_memory_mb > 0) {
        limiter_.ApplyMemoryLimit(
            static_cast<size_t>(policy_.max_memory_mb) * 1024 * 1024);
    }
    if (policy_.max_cpu_time_sec > 0) {
        limiter_.ApplyCpuTimeLimit(policy_.max_cpu_time_sec);
    }

    const std::string cmd = script_path + " 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        result.exit_code = -1;
        result.stderr_output = "popen failed";
        return result;
    }

    std::array<char, 256> buf{};
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        result.stdout_output += buf.data();
    }

    result.exit_code = pclose(pipe);
    return result;
}

}  // namespace EngineDoctor
