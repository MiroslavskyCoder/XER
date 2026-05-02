#pragma once
#include "system_analysis/base/os_detector_interface.h"
#include "system_analysis/base/common_system_info.h"
#include <string>
#include <vector>

namespace EngineDoctor {

struct SystemReport {
    OsType os;
    CommonSystemInfo info;
    std::vector<std::string> warnings;
};

class SystemAnalyzer {
public:
    virtual ~SystemAnalyzer() = default;
    virtual SystemReport Analyze() = 0;
};

} // namespace EngineDoctor
