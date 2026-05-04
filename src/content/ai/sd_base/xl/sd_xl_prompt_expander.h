#pragma once

#include <string>

namespace Engine::AI::SDBase {

class SdXlPromptExpander {
public:
    std::string Expand(const std::string& prompt) const;
};

}  // namespace Engine::AI::SDBase
