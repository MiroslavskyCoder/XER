#pragma once

#include "../model_core/model.h"

#include <string>

namespace Engine::ModelsBuilder::Operations {

class ModelCompressor {
 public:
	bool CompressModelToFile(const Core::Model& model, const std::string& output_path) const;
	std::string CompressModelToString(const Core::Model& model) const;
};

}  // namespace Engine::ModelsBuilder::Operations

