#pragma once
#include <string>

namespace EngineDoctor {

struct PackageDependency {
    std::string name;
    std::string version;
    bool optional = false;
};

} // namespace EngineDoctor
