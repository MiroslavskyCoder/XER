#pragma once

#include "classification/base_classifier.h"

namespace EngineDoctor {

/// Matches zero-byte files.
class No0FileClassifier : public BaseClassifier {
public:
    bool classify(const std::string& file_path, ClassificationResult& result) const override;
    std::string name() const override;
};

} // namespace EngineDoctor
