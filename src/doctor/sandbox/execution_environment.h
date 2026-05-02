#pragma once
#include <string>
#include <unordered_map>

namespace EngineDoctor {

struct ResourceLimits {
    size_t max_memory_bytes = 0;
    int cpu_time_sec = 0;
};

class ExecutionEnvironment {
public:
    std::string working_dir;
    std::unordered_map<std::string, std::string> env_vars;
    ResourceLimits limits;

    void SetEnv(const std::string& key, const std::string& val);
    std::string GetEnv(const std::string& key) const;
};

}  // namespace EngineDoctor
