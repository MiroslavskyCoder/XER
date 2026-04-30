#pragma once

#include <string>
#include <vector>
#include "classification/classification_result.h"

namespace EngineDoctor {

struct ClassificationRule {
	std::string name;
	std::string description;
	virtual ~ClassificationRule() = default;
	virtual bool match(const std::string& file_path, ClassificationResult& result) const = 0;
};

} // namespace EngineDoctor
