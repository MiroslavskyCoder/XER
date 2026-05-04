#pragma once

#include "content/ai/sd_base/core/sd_base_config.h"
#include "content/ai/sd_base/core/sd_base_types.h"

#include <memory>

namespace Engine::AI::SDBase {

class SdPipeline;

class SdBaseEngine {
public:
    explicit SdBaseEngine(SdBaseConfig config = SdBaseConfig{});
    ~SdBaseEngine();

    SdGenerationResult Generate(const SdGenerationRequest& request);

private:
    std::unique_ptr<SdPipeline> pipeline_;
};

}  // namespace Engine::AI::SDBase
