#pragma once

#include <cstdint>
#include <string>

#include "async_io/sync_primitives/mutex_wrapper.h"
 
#include <ffi.h> 

namespace Engine::Audio::Plugin {

class ClapHostInterface {
public:
    bool Initialize(double sample_rate, uint32_t max_block_size);
    bool LoadPlugin(const std::string& plugin_path);
    bool Process(const float* input, float* output, uint32_t frames);

private:
    IO::Sync::MutexWrapper mutex_{"clap_host"};
    double sample_rate_ = 44100.0;
    uint32_t max_block_size_ = 512;
    std::string plugin_path_;
};

}  // namespace Engine::Audio::Plugin
