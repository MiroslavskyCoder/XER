#pragma once
#include "sandbox/sandbox_policy.h"
#include "sandbox/resource_limiter.h"
#include <string>

namespace EngineDoctor {

struct ExecResult {
    int exit_code = -1;
    std::string stdout_output;
    std::string stderr_output;
    bool timed_out = false;
};

class SandboxExecutor {
public:
    explicit SandboxExecutor(const SandboxPolicy& policy);
    ExecResult Execute(const std::string& script_path);

private:
    SandboxPolicy policy_;
    ResourceLimiter limiter_;
};

}  // namespace EngineDoctor
