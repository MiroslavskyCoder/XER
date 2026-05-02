#include "package_handling/scan_allow_packages_arch.h"

namespace EngineDoctor {

void ScanAllowPackagesArch::AddAllowed(const std::string& name) {
    allowed_.insert(name);
}

bool ScanAllowPackagesArch::IsAllowed(const std::string& name) const {
    return allowed_.count(name) > 0;
}

}  // namespace EngineDoctor
