#pragma once

#include "content/ai/sd_base/core/sd_base_config.h"
#include "content/ai/sd_base/core/sd_base_scheduler.h"
#include "content/ai/sd_base/core/sd_base_types.h"

namespace Engine::AI::SDBase {

class SdPipeline {
public:
    explicit SdPipeline(SdBaseConfig config = SdBaseConfig{});

    SdGenerationResult Generate(const SdGenerationRequest& request);

private:
    SdBaseConfig config_;
    SdBaseScheduler scheduler_;
};

}  // namespace Engine::AI::SDBase
