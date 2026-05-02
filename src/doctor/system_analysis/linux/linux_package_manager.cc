#include "system_analysis/linux/linux_package_manager.h"
#include <cstdio>
#include <sstream>

namespace EngineDoctor {

namespace {

std::vector<PackageInfo> ParseDpkg(FILE* pipe) {
    std::vector<PackageInfo> packages;
    char line[1024];
    while (fgets(line, sizeof(line), pipe)) {
        std::string s(line);
        // dpkg -l lines start with "ii" for installed packages
        if (s.size() < 2 || s[0] != 'i' || s[1] != 'i') continue;
        std::istringstream iss(s.substr(2));
        PackageInfo pkg;
        iss >> pkg.name >> pkg.version >> pkg.arch;
        if (!pkg.name.empty()) {
            packages.push_back(std::move(pkg));
        }
    }
    return packages;
}

std::vector<PackageInfo> ParseRpm(FILE* pipe) {
    std::vector<PackageInfo> packages;
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
    return packages;
}

} // namespace

std::vector<PackageInfo> LinuxPackageManager::GetInstalledPackages() {
    // Try dpkg first
    FILE* pipe = popen("dpkg -l 2>/dev/null", "r");
    if (pipe) {
        auto pkgs = ParseDpkg(pipe);
        pclose(pipe);
        if (!pkgs.empty()) return pkgs;
    }

    // Fallback to rpm
    pipe = popen("rpm -qa --queryformat \"%{NAME} %{VERSION}\\n\" 2>/dev/null", "r");
    if (pipe) {
        auto pkgs = ParseRpm(pipe);
        pclose(pipe);
        return pkgs;
    }

    return {};
}

} // namespace EngineDoctor
