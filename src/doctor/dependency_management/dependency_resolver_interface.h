#pragma once

namespace EngineDoctor {

class DependencyResolverInterface {
public:
    virtual ~DependencyResolverInterface() = default;
    virtual bool Resolve() = 0;
};

} // namespace EngineDoctor
