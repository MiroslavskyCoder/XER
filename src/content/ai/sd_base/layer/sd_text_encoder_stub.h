#pragma once

#include "content/ai/ml/data_types/tensor.h"

#include <string>

namespace Engine::AI::SDBase {

class SdTextEncoderStub {
public:
    Engine::MLData::Types::Tensor Encode(const std::string& prompt,
                                         uint32_t max_tokens,
                                         uint32_t embedding_dim) const;
};

}  // namespace Engine::AI::SDBase
