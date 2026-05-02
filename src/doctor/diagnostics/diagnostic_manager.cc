#include "diagnostics/diagnostic_manager.h"
namespace EngineDoctor {
void DiagnosticManager::Attach(const std::string& name, std::function<std::string()> check) {
    checks_[name] = std::move(check);
}
std::unordered_map<std::string,std::string> DiagnosticManager::Collect() {
    std::unordered_map<std::string,std::string> results;
    for (const auto& [name, fn] : checks_) {
        results[name] = fn();
    }
    return results;
}
}
