#pragma once
#include "package_handling/package_fetcher.h"
#include <string>
#include <vector>

namespace EngineDoctor {

class LocalPackageSource {
public:
    explicit LocalPackageSource(const std::string& cache_dir);
    std::vector<PackageInfo> ListAvailable();

private:
    std::string cache_dir_;
};

}  // namespace EngineDoctor
