#pragma once

#include <cstdint>
#include <vector>

namespace Engine::AI::SDBase {

class SdBaseScheduler {
public:
    void BuildLinearSchedule(uint32_t steps, float beta_start = 0.00085f, float beta_end = 0.012f);

    uint32_t Steps() const { return steps_; }
    float BetaAt(uint32_t t) const;
    float AlphaCumProdAt(uint32_t t) const;
    float SigmaAt(uint32_t t) const;

private:
    uint32_t steps_ = 0;
    std::vector<float> betas_;
    std::vector<float> alphas_cumprod_;
};

}  // namespace Engine::AI::SDBase
