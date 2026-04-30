#pragma once

#include <vector>
#include <memory>
#include "classification/base_classifier.h"

namespace EngineDoctor {

class ClassifierManager {
public:
	void register_classifier(std::shared_ptr<BaseClassifier> classifier);
	bool classify(const std::string& file_path, ClassificationResult& result) const;

private:
	std::vector<std::shared_ptr<BaseClassifier>> classifiers_;
};

} // namespace EngineDoctor
