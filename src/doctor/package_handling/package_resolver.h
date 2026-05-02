#pragma once
#include "package_handling/package_fetcher.h"
#include <string>
#include <vector>

namespace EngineDoctor {

class PackageResolver {
public:
    std::vector<PackageInfo> Resolve(const std::string& name);
};

}  // namespace EngineDoctor
