#pragma once

#include "../model_core/model.h"

#include <string>

namespace Engine::ModelsBuilder::HardwareBinding {

class OnnxExporter {
 public:
	bool IsAvailable() const;
	bool ExportModel(const Core::Model& model, const std::string& output_path) const;
	std::string BuildOnnxLikeText(const Core::Model& model) const;
};

}  // namespace Engine::ModelsBuilder::HardwareBinding

