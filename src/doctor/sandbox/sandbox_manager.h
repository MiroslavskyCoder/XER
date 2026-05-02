#pragma once
#include "sandbox/sandbox_policy.h"

namespace EngineDoctor {

class SandboxManager {
public:
    bool Create(const SandboxPolicy& policy);
    bool Destroy();
    bool IsActive() const;

private:
    bool active_ = false;
    SandboxPolicy policy_;
};

}  // namespace EngineDoctor
