#include "no_0_file_classifier.h"

#include <filesystem>
#include <system_error>

namespace EngineDoctor {

std::string No0FileClassifier::name() const {
    return "No0FileClassifier";
}

bool No0FileClassifier::classify(const std::string& file_path, ClassificationResult& result) const {
    std::error_code ec;
    const auto size = std::filesystem::file_size(file_path, ec);
    if (ec) {
        result.status = ClassificationStatus::ERROR;
        result.error_message = ec.message();
        return false;
    }
    if (size == 0) {
        result.matched_rules.push_back("no_0_file");
        result.status = ClassificationStatus::MATCHED;
        return true;
    }
    result.status = ClassificationStatus::NOT_MATCHED;
    return false;
}

} // namespace EngineDoctor
