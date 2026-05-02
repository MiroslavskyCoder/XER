#include "package_handling/filter_engine_pack_a.h"

namespace EngineDoctor {

void FilterEnginePackA::AddFilter(std::shared_ptr<PackageFilter> filter) {
    filters_.emplace_back(std::move(filter));
}

std::vector<PackageInfo> FilterEnginePackA::Apply(
    const std::vector<PackageInfo>& packages) {
    std::vector<PackageInfo> result;
    for (const auto& pkg : packages) {
        bool matches = true;
        for (const auto& f : filters_) {
            if (!f->Matches(pkg)) {
                matches = false;
                break;
            }
        }
        if (matches) result.push_back(pkg);
    }
    return result;
}

}  // namespace EngineDoctor
