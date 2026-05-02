#pragma once
#include "package_handling/package_filter.h"
#include "package_handling/package_fetcher.h"
#include <memory>
#include <vector>

namespace EngineDoctor {

class FilterEnginePackA {
public:
    void AddFilter(std::shared_ptr<PackageFilter> filter);
    std::vector<PackageInfo> Apply(const std::vector<PackageInfo>& packages);

private:
    std::vector<std::shared_ptr<PackageFilter>> filters_;
};

}  // namespace EngineDoctor
