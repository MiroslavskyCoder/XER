#pragma once
#include <string>
#include <vector>

namespace EngineDoctor {

struct DependencyInfo {
    std::string file;
    std::vector<std::string> includes;
    std::vector<std::string> libs;
};

} // namespace EngineDoctor
