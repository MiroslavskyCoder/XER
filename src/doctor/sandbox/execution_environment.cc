#include "sandbox/execution_environment.h"

namespace EngineDoctor {

void ExecutionEnvironment::SetEnv(const std::string& key, const std::string& val) {
    env_vars[key] = val;
}

std::string ExecutionEnvironment::GetEnv(const std::string& key) const {
    auto it = env_vars.find(key);
    return (it != env_vars.end()) ? it->second : "";
}

}  // namespace EngineDoctor
