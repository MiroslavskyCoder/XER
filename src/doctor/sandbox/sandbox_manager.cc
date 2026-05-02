#include "sandbox/sandbox_manager.h"

namespace EngineDoctor {

bool SandboxManager::Create(const SandboxPolicy& policy) {
    if (active_) return false;
    policy_ = policy;
    active_ = true;
    return true;
}

bool SandboxManager::Destroy() {
    if (!active_) return false;
    active_ = false;
    return true;
}

bool SandboxManager::IsActive() const {
    return active_;
}

}  // namespace EngineDoctor
