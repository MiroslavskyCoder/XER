#pragma once

#include "sd_base_types.h"

#include <string>

namespace Engine::AI::SDBase {

struct SdBaseConfig {
    uint32_t max_width = 1024;
    uint32_t max_height = 1024;
    uint32_t min_steps = 1;
    uint32_t max_steps = 120;
    uint32_t default_steps = 30;
    uint32_t text_embedding_dim = 768;
    float init_noise_sigma = 1.0f;
    float eta = 0.0f;
    bool enable_cpu_fallback = true;
    bool prefer_cuda = true;

    // Backend preferences in descending priority.
    bool prefer_cudnn = true;
    bool prefer_cutlass = true;
    bool prefer_openvino = true;
    bool prefer_onnx = true;
    bool prefer_xnnpack = true;
    bool prefer_eigen = true;
    bool prefer_tensorflow = false;

    // Pipeline feature toggles.
    bool enable_sdxl = true;
    bool enable_controlnet = true;
    bool enable_vae = true;

    bool Validate(std::string* error) const;

    static SdBaseConfig FromPreset(SdQualityPreset preset);
};

}  // namespace Engine::AI::SDBase
