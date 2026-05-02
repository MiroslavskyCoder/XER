#pragma once
#include "system_analysis/base/system_analyzer.h"
#include <string>
#include <vector>

namespace EngineDoctor {

struct RequirementResult {
    bool passed;
    std::vector<std::string> failures;
};

class SystemRequirementChecker {
public:
    explicit SystemRequirementChecker(const SystemReport& report);
    RequirementResult Check();

private:
    const SystemReport& report_;
};

} // namespace EngineDoctor
