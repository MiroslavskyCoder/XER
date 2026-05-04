#pragma once

#include "content/ai/ml/data_types/tensor.h"
#include "content/ai/sd_base/core/sd_base_types.h"
#include "content/ai/sd_base/gpu/sd_gpu_dispatch.h"

namespace Engine::AI::SDBase {

class SdControlNetBranch {
public:
    void Apply(Engine::MLData::Types::Tensor& latent,
               const SdGenerationRequest& request,
               SdBackendKind backend,
               bool model_loaded) const;
};

}  // namespace Engine::AI::SDBase
