#include "dependency_management/v8_conflict_resolver.h"

namespace EngineDoctor {

std::vector<std::string> V8ConflictResolver::FindConflicts(
        const std::vector<PackageDependency>& deps) {
    std::vector<std::string> conflicts;
    std::string v8_version;

    for (const auto& dep : deps) {
        if (dep.name == "v8") {
            if (v8_version.empty()) {
                v8_version = dep.version;
            } else if (dep.version != v8_version) {
                conflicts.push_back(
                    "Conflicting V8 symbol definitions: version " +
                    v8_version + " vs " + dep.version);
            }
        }
    }

    return conflicts;
}

} // namespace EngineDoctor
