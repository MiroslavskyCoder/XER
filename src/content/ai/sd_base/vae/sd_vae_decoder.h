#pragma once

#include "content/ai/ml/data_types/tensor.h"

namespace Engine::AI::SDBase {

class SdVaeDecoder {
public:
    Engine::MLData::Types::Tensor Decode(const Engine::MLData::Types::Tensor& latent,
                                         uint32_t width,
                                         uint32_t height,
                                         bool high_quality) const;
};

}  // namespace Engine::AI::SDBase
