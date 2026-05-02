#include "null_empty_classifier.h"

namespace EngineDoctor {

std::string NullEmptyClassifier::name() const {
    return "NullEmptyClassifier";
}

bool NullEmptyClassifier::classify(const std::string& file_path, ClassificationResult& result) const {
    if (file_path.empty()) {
        result.matched_rules.push_back("null_empty");
        result.status = ClassificationStatus::MATCHED;
        return true;
    }
    result.status = ClassificationStatus::NOT_MATCHED;
    return false;
}

} // namespace EngineDoctor
