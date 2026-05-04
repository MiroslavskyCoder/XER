#pragma once

#include "content/ai/ml/data_types/tensor.h"
#include "content/ai/ml/utils/ml_random_generator.h"

#include <cstdint>

namespace Engine::AI::SDBase {

class SdLatentTensor {
public:
    static Engine::MLData::Types::Tensor Make(uint32_t width, uint32_t height);
    static void FillGaussian(Engine::MLData::Types::Tensor& latent,
                             Engine::ML::Utils::MlRandomGenerator& rng,
                             float sigma);
};

}  // namespace Engine::AI::SDBase
