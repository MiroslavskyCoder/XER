#pragma once

#include "../model_core/model.h"

#include <string>

namespace Engine::ModelsBuilder::Operations {

class ModelExporter {
 public:
	bool ExportNative(const Core::Model& model, const std::string& output_path) const;
	bool ExportOnnxLike(const Core::Model& model, const std::string& output_path) const;
};

}  // namespace Engine::ModelsBuilder::Operations

