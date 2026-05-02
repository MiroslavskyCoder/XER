#include "dependency_management/conflict_detector.h"

namespace EngineDoctor {

std::vector<Conflict> ConflictDetector::Detect(const DependencyGraph& graph) {
    std::vector<Conflict> conflicts;

    const auto nodes = graph.GetAllNodes();

    for (const auto& a : nodes) {
        for (const auto& b : graph.GetDependencies(a)) {
            // Circular dep: A -> B and B -> A
            if (graph.HasEdge(b, a)) {
                // Avoid duplicate (a<->b already recorded as b<->a)
                bool dup = false;
                for (const auto& existing : conflicts) {
                    if (existing.node_a == b && existing.node_b == a) {
                        dup = true;
                        break;
                    }
                }
                if (!dup) {
                    conflicts.push_back({a, b, "Circular dependency: " + a + " <-> " + b});
                }
            }
        }
    }

    return conflicts;
}

} // namespace EngineDoctor
