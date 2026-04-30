#include "classification/classifier_manager.h"

namespace EngineDoctor {

void ClassifierManager::register_classifier(std::shared_ptr<BaseClassifier> classifier) {
	classifiers_.push_back(std::move(classifier));
}

bool ClassifierManager::classify(const std::string& file_path, ClassificationResult& result) const {
	for (const auto& classifier : classifiers_) {
		if (classifier->classify(file_path, result)) {
			return true;
		}
	}
	return false;
}

} // namespace EngineDoctor
