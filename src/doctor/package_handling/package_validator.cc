#include "package_handling/package_validator.h"

namespace EngineDoctor {

ValidationResult PackageValidator::Validate(const PackageInfo& pkg) {
    ValidationResult result;
    result.valid = true;
    if (pkg.name.empty()) {
        result.valid = false;
        result.errors.emplace_back("Package name is empty");
    }
    if (pkg.version.empty()) {
        result.valid = false;
        result.errors.emplace_back("Package version is empty");
    }
    return result;
}

}  // namespace EngineDoctor
