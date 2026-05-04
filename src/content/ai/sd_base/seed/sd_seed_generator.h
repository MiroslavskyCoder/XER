#pragma once

#include <cstdint>
#include <string>

namespace Engine::AI::SDBase {

class SdSeedGenerator {
public:
    static uint64_t FromPrompt(const std::string& prompt,
                               const std::string& negative_prompt,
                               uint64_t fallback_seed = 0);
};

}  // namespace Engine::AI::SDBase
