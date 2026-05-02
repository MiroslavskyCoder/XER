#pragma once
#include <string>
#include <vector>

namespace EngineDoctor {

struct PackageInfo {
    std::string name;
    std::string version;
    std::string arch;
};

class LinuxPackageManager {
public:
    std::vector<PackageInfo> GetInstalledPackages();
};

} // namespace EngineDoctor
