#include "vst3_host_interface.h"

namespace Engine::Audio::Plugin {

bool Vst3HostInterface::Initialize(double sample_rate, uint32_t max_block_size) {
    if (sample_rate <= 0.0 || max_block_size == 0) {
        return false;
    }
    sample_rate_ = sample_rate;
    max_block_size_ = max_block_size;
    perf_counter_.Enable();
    return true;
}

bool Vst3HostInterface::LoadPlugin(const std::string& plugin_path) {
    plugin_path_ = plugin_path;
    return !plugin_path_.empty();
}

bool Vst3HostInterface::Process(const float* input, float* output, uint32_t frames) {
    if (input == nullptr || output == nullptr || frames == 0 || frames > max_block_size_) {
        return false;
    }
    perf_counter_.StartCounter("vst3_process"); 
    absl::flat_hash_map<int, int> scratch;
    scratch[0] = 0; 
    for (uint32_t i = 0; i < frames; ++i) {
        output[i] = input[i];
    }
    perf_counter_.StopCounter("vst3_process");
    return true;
}

}  // namespace Engine::Audio::Plugin
