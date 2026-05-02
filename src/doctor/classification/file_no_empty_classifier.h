#pragma once

#include "classification/base_classifier.h"

namespace EngineDoctor {

/// Matches non-empty files (file_size > 0).
class FileNoEmptyClassifier : public BaseClassifier {
public:
    bool classify(const std::string& file_path, ClassificationResult& result) const override;
    std::string name() const override;
};

} // namespace EngineDoctor
