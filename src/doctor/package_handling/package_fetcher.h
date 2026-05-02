#pragma once
#include <string>

namespace EngineDoctor {

struct PackageInfo {
    std::string name;
    std::string version;
    std::string arch;
    std::string description;
};

class PackageFetcher {
public:
    bool Fetch(const std::string& name, PackageInfo& out);
};

}  // namespace EngineDoctor
