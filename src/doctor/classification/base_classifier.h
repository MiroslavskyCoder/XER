#pragma once

#include <string>
#include "classification/classification_result.h"

namespace EngineDoctor {

class BaseClassifier {
public:
	virtual ~BaseClassifier() = default;
	virtual bool classify(const std::string& file_path, ClassificationResult& result) const = 0;
	virtual std::string name() const = 0;
};

} // namespace EngineDoctor
