#pragma once

#include "content/ai/ml/data_types/tensor.h"

namespace Engine::AI::SDBase {

class SdControlNetRouter {
public:
    void ApplyHintScale(Engine::MLData::Types::Tensor& latent, float hint_strength) const;
};

}  // namespace Engine::AI::SDBase
