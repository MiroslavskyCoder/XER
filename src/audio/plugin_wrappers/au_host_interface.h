#pragma once

#include <cstdint>
#include <string>

#include "async_io/sync_primitives/mutex_wrapper.h"
#include "plugin_builtin_host.h"

namespace Engine::Audio::Plugin {

class AuHostInterface {
public:
    bool Initialize(double sample_rate, uint32_t max_block_size);
    bool LoadComponent(const std::string& component_id);
    bool Process(const float* input, float* output, uint32_t frames);
    bool SetParameter(uint32_t id, float value);
    bool GetParameter(uint32_t id, float* value) const;
    std::vector<PluginParameterInfo> GetParameters() const;
    std::string GetLoadedComponentId() const;

private:
    mutable AsyncIO::IO::Sync::MutexWrapper mutex_{"au_host"};
    BuiltinPluginHost builtin_host_;
    double sample_rate_ = 44100.0;
    uint32_t max_block_size_ = 512;
    std::string component_id_;
};

}  // namespace Engine::Audio::Plugin
