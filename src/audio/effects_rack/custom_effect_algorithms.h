#pragma once

#include <string>
#include <vector>

#include "custom_effect_struct.h"

namespace Engine::Audio::FX {

std::vector<std::string> ListCustomEffectAlgorithms();
CustomEffectNode CreateDefaultCustomEffectNode(const std::string& algorithm_name);

}  // namespace Engine::Audio::FX
