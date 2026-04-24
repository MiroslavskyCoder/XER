#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "async_io/sync_primitives/mutex_wrapper.h"
#include "audio/midi_sequencing/midi_controller_mapping.h"

namespace Engine::Audio::Plugin {

class PluginParameterBridge {
public:
    void RegisterParameter(uint32_t id, const std::string& name, float default_value);
    bool SetValue(uint32_t id, float value);
    bool GetValue(uint32_t id, float& value) const;

private:
    mutable AsyncIO::IO::Sync::MutexWrapper mutex_{"plugin_param_bridge"};
    std::unordered_map<uint32_t, float> values_;
};

}  // namespace Engine::Audio::Plugin
