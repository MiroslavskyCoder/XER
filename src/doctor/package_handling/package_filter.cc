#include "package_handling/package_filter.h"

namespace EngineDoctor {

void PackageFilter::SetNamePattern(const std::string& pattern) {
    name_pattern_ = pattern;
}

void PackageFilter::SetMinVersion(const std::string& version) {
    min_version_ = version;
}

bool PackageFilter::Matches(const PackageInfo& pkg) const {
    if (!name_pattern_.empty() &&
        pkg.name.find(name_pattern_) == std::string::npos) {
        return false;
    }
    if (!min_version_.empty() && pkg.version < min_version_) {
        return false;
    }
    return true;
}

}  // namespace EngineDoctor
