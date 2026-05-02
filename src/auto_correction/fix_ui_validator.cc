#include "fix_ui_validator.h"

namespace AutoCorrection {

ValidationResult FixUiValidator::Validate(const std::string& input) {
    ValidationResult result;

    if (input.empty()) {
        result.valid = false;
        result.errors.push_back("Input must not be empty");
        return result;
    }

    if (input.size() > 1024 * 1024) {
        result.valid = false;
        result.errors.push_back("Input exceeds maximum allowed size (1 MB)");
    }

    // Reject null bytes
    if (input.find('\0') != std::string::npos) {
        result.valid = false;
        result.errors.push_back("Input contains null bytes");
    }

    return result;
}

} // namespace AutoCorrection
