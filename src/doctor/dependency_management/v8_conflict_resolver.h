#pragma once
#include "dependency_management/package_dependency.h"
#include <string>
#include <vector>

namespace EngineDoctor {

class V8ConflictResolver {
public:
    std::vector<std::string> FindConflicts(
        const std::vector<PackageDependency>& deps);
};

} // namespace EngineDoctor
