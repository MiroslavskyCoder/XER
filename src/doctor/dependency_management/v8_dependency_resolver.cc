#include "dependency_management/v8_dependency_resolver.h"

namespace EngineDoctor {

V8DependencyResolver::V8DependencyResolver(std::vector<PackageDependency> deps)
    : deps_(std::move(deps)) {}

bool V8DependencyResolver::Resolve() {
    for (const auto& dep : deps_) {
        if (dep.name == "v8") return true;
    }
    return false;
}

} // namespace EngineDoctor
