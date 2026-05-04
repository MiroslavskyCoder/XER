#pragma once

#include "content/ai/ml/data_types/tensor.h"

namespace Engine::AI::SDBase {

class SdDepthConditioner {
public:
    void ApplyDepthPrior(Engine::MLData::Types::Tensor& latent, float depth_weight) const;
};

}  // namespace Engine::AI::SDBase
