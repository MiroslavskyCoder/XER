// PXER Flux extension integration (universal template)
#pragma once
#include "native/plugin/pxer/pxer_host.h"
#include <string>
#include <unordered_map>

namespace flux {

// Call this during Flux system initialization
inline void RegisterPxerFluxExtensions() {
    auto& pxer = Engine::Native::Plugin::Pxer::PluginHost::Shared();
    // Register all PXER plugin types
    for (const auto& entry : pxer.GetFluxTypes()) {
        const std::string& type_name = entry.first;
        void* type_info = entry.second;
        // TODO: Replace with your real Flux type registration logic:
        // Example: FluxTypeRegistry::Instance().Register(type_name, type_info);
    }
    // Register all PXER plugin triggers
    for (const auto& entry : pxer.GetFluxTriggers()) {
        const std::string& trigger_name = entry.first;
        void* trigger_info = entry.second;
        // TODO: Replace with your real Flux trigger registration logic:
        // Example: FluxTriggerRegistry::Instance().Register(trigger_name, trigger_info);
    }
}

} // namespace flux
