#pragma once

#include <cstdint>
#include <string>

#include "async_io/log_and_debug/io_perf_counter.h"
 
#include <absl/container/flat_hash_map.h> 

namespace Engine::Audio::Plugin {

class Vst3HostInterface {
public:
    bool Initialize(double sample_rate, uint32_t max_block_size);
    bool LoadPlugin(const std::string& plugin_path);
    bool Process(const float* input, float* output, uint32_t frames);

private:
    AsyncIO::IO::LogDebug::PerformanceCounter perf_counter_;
    double sample_rate_ = 44100.0;
    uint32_t max_block_size_ = 512;
    std::string plugin_path_;
};

}  // namespace Engine::Audio::Plugin
