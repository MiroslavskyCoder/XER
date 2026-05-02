#pragma once

#include "classification/base_classifier.h"

namespace EngineDoctor {

/// Matches crash dump paths whose filename contains "SIGSEGV" or "FATAL".
class FatalStackErrorClassifier : public BaseClassifier {
public:
    bool classify(const std::string& file_path, ClassificationResult& result) const override;
    std::string name() const override;
};

} // namespace EngineDoctor
