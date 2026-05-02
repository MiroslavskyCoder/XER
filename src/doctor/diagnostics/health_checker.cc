#include "diagnostics/health_checker.h"
namespace EngineDoctor {
void HealthChecker::AddCheck(const std::string& name, std::function<bool()> check) {
    checks_[name] = std::move(check);
}
HealthReport HealthChecker::Run() {
    HealthReport report;
    report.status = HealthStatus::OK;
    for (const auto& [name, check] : checks_) {
        if (!check()) {
            report.issues.push_back(name + ": FAILED");
            report.status = HealthStatus::DEGRADED;
        }
    }
    return report;
}
}
