#pragma once
#include "package_handling/package_fetcher.h"
#include <string>

namespace EngineDoctor {

class PackageFilter {
public:
    void SetNamePattern(const std::string& pattern);
    void SetMinVersion(const std::string& version);
    bool Matches(const PackageInfo& pkg) const;

private:
    std::string name_pattern_;
    std::string min_version_;
};

}  // namespace EngineDoctor
