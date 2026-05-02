#pragma once
#include "dependency_management/dependency_resolver_interface.h"
#include "dependency_management/package_dependency.h"
#include <vector>

namespace EngineDoctor {

class V8DependencyResolver : public DependencyResolverInterface {
public:
    explicit V8DependencyResolver(std::vector<PackageDependency> deps);
    bool Resolve() override;

private:
    std::vector<PackageDependency> deps_;
};

} // namespace EngineDoctor
