#pragma once

#include "content/ai/ml/data_types/tensor.h"

namespace Engine::AI::SDBase {

class SdImageDecoder {
public:
    Engine::MLData::Types::Tensor DecodeLatentToImage(const Engine::MLData::Types::Tensor& latent,
                                                      uint32_t width,
                                                      uint32_t height) const;
};

}  // namespace Engine::AI::SDBase
