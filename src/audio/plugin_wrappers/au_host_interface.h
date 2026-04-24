#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "async_io/sync_primitives/mutex_wrapper.h"

namespace Engine::Audio::Plugin {

class AuHostInterface {
public:
    bool Initialize(double sample_rate, uint32_t max_block_size);
    bool LoadComponent(const std::string& component_id);
    bool Process(const float* input, float* output, uint32_t frames);

private:
    IO::Sync::MutexWrapper mutex_{"au_host"};
    double sample_rate_ = 44100.0;
    uint32_t max_block_size_ = 512;
    std::string component_id_;
};

}  // namespace Engine::Audio::Plugin
