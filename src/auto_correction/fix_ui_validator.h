#pragma once

#include <string>
#include <vector>

namespace AutoCorrection {

struct ValidationResult {
    bool valid = true;
    std::vector<std::string> errors;
};

class FixUiValidator {
public:
    ValidationResult Validate(const std::string& input);
};

} // namespace AutoCorrection
