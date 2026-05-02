#pragma once
#include "system_analysis/linux/linux_package_manager.h"
#include <vector>

namespace EngineDoctor {

class OpenBsdPackageManager {
public:
    std::vector<PackageInfo> GetInstalledPackages();
};

} // namespace EngineDoctor
