#pragma once
#include "package_handling/package_fetcher.h"
#include <string>
#include <vector>

namespace EngineDoctor {

struct ValidationResult {
    bool valid;
    std::vector<std::string> errors;
};

class PackageValidator {
public:
    ValidationResult Validate(const PackageInfo& pkg);
};

}  // namespace EngineDoctor
