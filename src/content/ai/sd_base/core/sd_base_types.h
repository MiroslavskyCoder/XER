#pragma once

#include "content/ai/ml/data_types/tensor.h"

#include <cstdint>
#include <map>
#include <string>

namespace Engine::AI::SDBase {

enum class SdQualityPreset : uint8_t {
    Draft = 0,
    Balanced = 1,
    High = 2,
};

struct SdRuntimeCapabilities {
    bool cuda_available = false;
    bool opencv_available = false;
    bool ffmpeg_available = false;
    bool eigen_available = false;
    bool xnnpack_available = false;
};

struct SdGenerationRequest {
    std::string prompt;
    std::string negative_prompt;
    uint32_t width = 512;
    uint32_t height = 512;
    uint32_t steps = 30;
    float guidance_scale = 7.5f;
    uint64_t seed = 0;
    SdQualityPreset preset = SdQualityPreset::Balanced;
};

struct SdGenerationResult {
    bool ok = false;
    std::string error;
    Engine::MLData::Types::Tensor latent = Engine::MLData::Types::Tensor({1, 4, 64, 64});
    Engine::MLData::Types::Tensor image = Engine::MLData::Types::Tensor({1, 3, 512, 512});
    std::map<std::string, std::string> metadata;
};

}  // namespace Engine::AI::SDBase
