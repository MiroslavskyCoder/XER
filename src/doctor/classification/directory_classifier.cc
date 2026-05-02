#include "directory_classifier.h"

#include <filesystem>

namespace EngineDoctor {

std::string DirectoryClassifier::name() const {
    return "DirectoryClassifier";
}

bool DirectoryClassifier::classify(const std::string& file_path, ClassificationResult& result) const {
    if (std::filesystem::is_directory(file_path)) {
        result.matched_rules.push_back("directory");
        result.status = ClassificationStatus::MATCHED;
        return true;
    }
    result.status = ClassificationStatus::NOT_MATCHED;
    return false;
}

} // namespace EngineDoctor
