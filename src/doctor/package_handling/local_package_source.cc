#include "package_handling/local_package_source.h"
#include <filesystem>
#include <system_error>

namespace EngineDoctor {

LocalPackageSource::LocalPackageSource(const std::string& cache_dir)
    : cache_dir_(cache_dir) {}

std::vector<PackageInfo> LocalPackageSource::ListAvailable() {
    std::vector<PackageInfo> result;
    std::error_code ec;
    for (const auto& entry :
         std::filesystem::directory_iterator(cache_dir_, ec)) {
        if (ec) break;
        if (!entry.is_regular_file(ec)) continue;
        const std::string ext = entry.path().extension().string();
        if (ext != ".gz") continue;
        const std::string stem = entry.path().stem().string();
        if (stem.size() < 4 ||
            stem.substr(stem.size() - 4) != ".tar")
            continue;
        PackageInfo pkg;
        pkg.name = entry.path().filename().string();
        result.push_back(std::move(pkg));
    }
    return result;
}

}  // namespace EngineDoctor
