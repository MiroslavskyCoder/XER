// PXER Flux extension integration
#pragma once
#include "native/plugin/pxer/pxer_host.h"
#include <string>
#include <vector>

namespace flux {

// Enumerate Flux types and triggers registered by all currently-loaded PXER
// plugins and return them as string lists for further registration into
// whichever Flux registry the caller uses.
//
// Example:
//   auto [types, triggers] = CollectPxerFluxExtensions();
//   for (const auto& name : types)
//       FluxTypeRegistry::Instance().Register(name, ...);
inline std::pair<std::vector<std::string>, std::vector<std::string>>
CollectPxerFluxExtensions() {
    using namespace Engine::Native::Plugin::Pxer;
    auto& pxer = PluginHost::Shared();

    std::vector<std::string> all_types;
    std::vector<std::string> all_triggers;

    for (const PluginSummary& plugin : pxer.List()) {
        if (!plugin.loaded) continue;

        for (const std::string& type_name :
                pxer.ListFluxTypesByInstance(plugin.instance_id)) {
            all_types.push_back(type_name);
        }
        for (const std::string& trigger_name :
                pxer.ListFluxTriggersByInstance(plugin.instance_id)) {
            all_triggers.push_back(trigger_name);
        }
    }

    return {std::move(all_types), std::move(all_triggers)};
}

// Convenience overload: call CollectPxerFluxExtensions and register each
// name via the provided callbacks.
//
//   RegisterPxerFluxExtensions(
//       [](const std::string& name){ FluxTypeRegistry::Instance().Register(name); },
//       [](const std::string& name){ FluxTriggerRegistry::Instance().Register(name); });
template <typename TypeCb, typename TriggerCb>
inline void RegisterPxerFluxExtensions(TypeCb on_type, TriggerCb on_trigger) {
    auto [types, triggers] = CollectPxerFluxExtensions();
    for (const auto& name : types)    on_type(name);
    for (const auto& name : triggers) on_trigger(name);
}

} // namespace flux
