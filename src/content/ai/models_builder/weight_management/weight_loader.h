#pragma once

#include "weight_initialization.h"

#include <string>
#include <vector>

namespace Engine::ModelsBuilder::Weights {

class WeightLoader {
public:
	std::vector<WeightTensor> Load(const std::string& filepath) const;
};

} // namespace Engine::ModelsBuilder::Weights
