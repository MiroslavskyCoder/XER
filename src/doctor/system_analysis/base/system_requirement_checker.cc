#include "system_analysis/base/system_requirement_checker.h"

namespace EngineDoctor {

SystemRequirementChecker::SystemRequirementChecker(const SystemReport& report)
    : report_(report) {}

RequirementResult SystemRequirementChecker::Check() {
    RequirementResult result;
    result.passed = true;

    if (report_.info.kernel_version.empty()) {
        result.failures.push_back("Kernel version is empty.");
        result.passed = false;
    }

    return result;
}

} // namespace EngineDoctor
