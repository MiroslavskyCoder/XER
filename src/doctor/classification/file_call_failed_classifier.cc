#include "file_call_failed_classifier.h"

#include <filesystem>

namespace EngineDoctor {

std::string FileCallFailedClassifier::name() const {
    return "FileCallFailedClassifier";
}

bool FileCallFailedClassifier::classify(const std::string& file_path, ClassificationResult& result) const {
    if (!std::filesystem::exists(file_path)) {
        result.matched_rules.push_back("file_call_failed");
        result.status = ClassificationStatus::MATCHED;
        return true;
    }
    result.status = ClassificationStatus::NOT_MATCHED;
    return false;
}

} // namespace EngineDoctor
