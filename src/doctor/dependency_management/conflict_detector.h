#pragma once
#include "dependency_management/dependency_graph.h"
#include <string>
#include <vector>

namespace EngineDoctor {

struct Conflict {
    std::string node_a;
    std::string node_b;
    std::string reason;
};

class ConflictDetector {
public:
    std::vector<Conflict> Detect(const DependencyGraph& graph);
};

} // namespace EngineDoctor
