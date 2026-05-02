#include "no_correct_classifier.h"

namespace EngineDoctor {

std::string NoCorrectClassifier::name() const {
    return "NoCorrectClassifier";
}

bool NoCorrectClassifier::classify(const std::string& /*file_path*/, ClassificationResult& result) const {
    result.matched_rules.push_back("no_correct");
    result.status = ClassificationStatus::MATCHED;
    return true;
}

} // namespace EngineDoctor
