#include "validator.h"

namespace Engine::MLData::Validation {

DataValidator& DataValidator::GetInstance() {
    static DataValidator instance;
    return instance;
}

ValidationResult DataValidator::Validate(const Types::Dataset& dataset) {
    return {true, "", 1.0f};
}

ValidationResult DataValidator::ValidateBatch(const Types::Batch& batch) {
    return {true, "", 1.0f};
}

ValidationResult DataValidator::ValidateTensor(const Types::Tensor& tensor) {
    return {true, "", 1.0f};
}

} // namespace Engine::MLData::Validation
