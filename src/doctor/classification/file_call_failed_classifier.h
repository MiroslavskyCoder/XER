#pragma once

#include "classification/base_classifier.h"

namespace EngineDoctor {

/// Matches when a file open attempt would fail (path does not exist).
class FileCallFailedClassifier : public BaseClassifier {
public:
    bool classify(const std::string& file_path, ClassificationResult& result) const override;
    std::string name() const override;
};

} // namespace EngineDoctor
