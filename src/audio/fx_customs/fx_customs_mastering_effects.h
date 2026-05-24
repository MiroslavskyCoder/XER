#pragma once

#include <string>

#include "audio/effects_rack/custom_effect_struct.h"

namespace Engine::Audio::FX::Customs {

bool BuildFxCustomMasteringEffectPackage(const std::string& normalized_name, CustomEffectPackage* package_out);

}  // namespace Engine::Audio::FX::Customs
