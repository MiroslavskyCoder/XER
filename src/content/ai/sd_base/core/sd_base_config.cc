#include "sd_base_config.h"

namespace Engine::AI::SDBase {

bool SdBaseConfig::Validate(std::string* error) const {
    if (max_width < 64 || max_height < 64) {
        if (error) *error = "max resolution is too small";
        return false;
    }
    if (min_steps == 0 || max_steps < min_steps) {
        if (error) *error = "invalid diffusion step range";
        return false;
    }
    if (default_steps < min_steps || default_steps > max_steps) {
        if (error) *error = "default steps are outside the configured range";
        return false;
    }
    if (text_embedding_dim == 0) {
        if (error) *error = "text embedding dimension must be > 0";
        return false;
    }
    if (init_noise_sigma <= 0.0f) {
        if (error) *error = "initial noise sigma must be > 0";
        return false;
    }
    return true;
}

SdBaseConfig SdBaseConfig::FromPreset(SdQualityPreset preset) {
    SdBaseConfig cfg;
    switch (preset) {
        case SdQualityPreset::Draft:
            cfg.default_steps = 12;
            cfg.max_steps = 40;
            cfg.text_embedding_dim = 384;
            cfg.eta = 0.2f;
            break;
        case SdQualityPreset::Balanced:
            cfg.default_steps = 30;
            cfg.max_steps = 120;
            cfg.text_embedding_dim = 768;
            cfg.eta = 0.0f;
            break;
        case SdQualityPreset::High:
            cfg.default_steps = 50;
            cfg.max_steps = 160;
            cfg.text_embedding_dim = 1024;
            cfg.eta = 0.0f;
            break;
    }
    return cfg;
}

}  // namespace Engine::AI::SDBase
