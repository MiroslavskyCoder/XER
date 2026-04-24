#include "clap_host_interface.h"

namespace Engine::Audio::Plugin {

bool ClapHostInterface::Initialize(double sample_rate, uint32_t max_block_size) {
    if (sample_rate <= 0.0 || max_block_size == 0) {
        return false;
    }
    IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    sample_rate_ = sample_rate;
    max_block_size_ = max_block_size;
    return true;
}

bool ClapHostInterface::LoadPlugin(const std::string& plugin_path) {
    IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    plugin_path_ = plugin_path;
    return !plugin_path_.empty();
}

bool ClapHostInterface::Process(const float* input, float* output, uint32_t frames) {
    if (input == nullptr || output == nullptr || frames == 0 || frames > max_block_size_) {
        return false;
    }
    IO::Sync::MutexWrapper::ScopedLock lock(mutex_); 
    ffi_type* arg_types[1] = { &ffi_type_float };
    (void)arg_types; 
    for (uint32_t i = 0; i < frames; ++i) {
        output[i] = input[i];
    }
    return true;
}

}  // namespace Engine::Audio::Plugin
