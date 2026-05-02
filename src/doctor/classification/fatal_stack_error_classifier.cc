#include "fatal_stack_error_classifier.h"

#include <filesystem>
#include <string>

namespace EngineDoctor {

std::string FatalStackErrorClassifier::name() const {
    return "FatalStackErrorClassifier";
}

bool FatalStackErrorClassifier::classify(const std::string& file_path, ClassificationResult& result) const {
    const std::string filename = std::filesystem::path(file_path).filename().string();
    if (filename.find("SIGSEGV") != std::string::npos ||
        filename.find("FATAL")   != std::string::npos) {
        result.matched_rules.push_back("fatal_stack_error");
        result.status = ClassificationStatus::MATCHED;
        return true;
    }
    result.status = ClassificationStatus::NOT_MATCHED;
    return false;
}

} // namespace EngineDoctor
