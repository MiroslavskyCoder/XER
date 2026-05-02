#pragma once
#include <string>
#include <vector>

namespace EngineDoctor {

struct SandboxPolicy {
    std::vector<std::string> allowed_paths;
    std::vector<std::string> allowed_syscalls;
    int max_memory_mb = 256;
    int max_cpu_time_sec = 30;
    bool network_allowed = false;
};

}  // namespace EngineDoctor
