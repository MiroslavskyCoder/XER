#include "au_host_interface.h"

#include <algorithm>

namespace Engine::Audio::Plugin {

bool AuHostInterface::Initialize(double sample_rate, uint32_t max_block_size) {
    if (sample_rate <= 0.0 || max_block_size == 0) {
        return false;
    }
    IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    sample_rate_ = sample_rate;
    max_block_size_ = max_block_size;
    return true;
}

bool AuHostInterface::LoadComponent(const std::string& component_id) {
    IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    component_id_ = component_id;
    return !component_id_.empty();
}

bool AuHostInterface::Process(const float* input, float* output, uint32_t frames) {
    if (input == nullptr || output == nullptr || frames == 0 || frames > max_block_size_) {
        return false;
    }
    IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    for (uint32_t i = 0; i < frames; ++i) {
        output[i] = input[i];
    }
    return true;
}

}  // namespace Engine::Audio::Plugin
