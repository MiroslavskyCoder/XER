#pragma once

#include "../data_types/dataset.h"
#include <string>

namespace Engine::MLData::Validation {

struct ValidationResult {
    bool is_valid;
    std::string error_message;
    float completeness_score;
};

class DataValidator {
public:
    static DataValidator& GetInstance();
    
    ValidationResult Validate(const Types::Dataset& dataset);
    ValidationResult ValidateBatch(const Types::Batch& batch);
    ValidationResult ValidateTensor(const Types::Tensor& tensor);

private:
    DataValidator() = default;
};

} // namespace Engine::MLData::Validation
