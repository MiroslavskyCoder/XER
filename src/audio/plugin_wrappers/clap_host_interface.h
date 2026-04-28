#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "async_io/sync_primitives/mutex_wrapper.h"
#include "plugin_builtin_host.h"

namespace Engine::Audio::Plugin {

class ClapHostInterface {
public:
    bool Initialize(double sample_rate, uint32_t max_block_size);
    bool LoadPlugin(const std::string& plugin_path);
    bool Process(const float* input, float* output, uint32_t frames);
    bool SetParameter(uint32_t id, float value);
    bool GetParameter(uint32_t id, float* value) const;
    std::vector<PluginParameterInfo> GetParameters() const;
    std::string GetLoadedPluginId() const;

private:
    mutable AsyncIO::IO::Sync::MutexWrapper mutex_{"clap_host"};
    BuiltinPluginHost builtin_host_;
    double sample_rate_ = 44100.0;
    uint32_t max_block_size_ = 512;
    std::string plugin_path_;
};

}  // namespace Engine::Audio::Plugin
