#include "system_analysis/macos/macos_package_manager.h"
#include <cstdio>
#include <sstream>

namespace EngineDoctor {

std::vector<PackageInfo> MacOsPackageManager::GetInstalledPackages() {
    std::vector<PackageInfo> packages;

    FILE* pipe = popen("brew list --versions 2>/dev/null", "r");
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
