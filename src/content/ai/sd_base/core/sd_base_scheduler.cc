#include "sd_base_scheduler.h"

#include <algorithm>
#include <cmath>

namespace Engine::AI::SDBase {

void SdBaseScheduler::BuildLinearSchedule(uint32_t steps, float beta_start, float beta_end) {
    steps_ = std::max<uint32_t>(1, steps);
    betas_.assign(steps_, 0.0f);
    alphas_cumprod_.assign(steps_, 0.0f);

    const float step_inv = (steps_ > 1) ? 1.0f / static_cast<float>(steps_ - 1) : 0.0f;
    float running = 1.0f;

    for (uint32_t i = 0; i < steps_; ++i) {
        const float u = static_cast<float>(i) * step_inv;
        const float beta = beta_start + (beta_end - beta_start) * u;
        betas_[i] = std::clamp(beta, 1e-6f, 0.999f);

        const float alpha = 1.0f - betas_[i];
        running *= alpha;
        alphas_cumprod_[i] = std::clamp(running, 1e-8f, 1.0f);
    }
}

float SdBaseScheduler::BetaAt(uint32_t t) const {
    if (betas_.empty()) return 0.0f;
    const uint32_t idx = std::min<uint32_t>(t, static_cast<uint32_t>(betas_.size() - 1));
    return betas_[idx];
}

float SdBaseScheduler::AlphaCumProdAt(uint32_t t) const {
    if (alphas_cumprod_.empty()) return 1.0f;
    const uint32_t idx = std::min<uint32_t>(t, static_cast<uint32_t>(alphas_cumprod_.size() - 1));
    return alphas_cumprod_[idx];
}

float SdBaseScheduler::SigmaAt(uint32_t t) const {
    const float a = AlphaCumProdAt(t);
    const float one_minus_a = std::max(1e-8f, 1.0f - a);
    return std::sqrt(one_minus_a / a);
}

}  // namespace Engine::AI::SDBase
