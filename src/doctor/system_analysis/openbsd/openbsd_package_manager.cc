#include "system_analysis/openbsd/openbsd_package_manager.h"
#include <cstdio>
#include <sstream>

namespace EngineDoctor {

std::vector<PackageInfo> OpenBsdPackageManager::GetInstalledPackages() {
    std::vector<PackageInfo> packages;

    FILE* pipe = popen("pkg_info 2>/dev/null", "r");
    if (!pipe) return packages;

    char line[1024];
    while (fgets(line, sizeof(line), pipe)) {
        std::string s(line);
        if (!s.empty() && s.back() == '\n') s.pop_back();
        std::istringstream iss(s);
        PackageInfo pkg;
        iss >> pkg.name >> pkg.version;
        if (!pkg.name.empty()) {
            packages.push_back(std::move(pkg));
        }
    }

    pclose(pipe);
    return packages;
}

} // namespace EngineDoctor
