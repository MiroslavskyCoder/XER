#pragma once

#include "content/ai/sd_base/core/sd_base_types.h"

namespace Engine::AI::SDBase {

class SdGpuDispatch {
public:
    static SdRuntimeCapabilities Detect();
    static bool ShouldUseCuda(const SdRuntimeCapabilities& caps, bool prefer_cuda);
};

}  // namespace Engine::AI::SDBase
