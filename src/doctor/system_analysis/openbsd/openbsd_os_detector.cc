#include "system_analysis/openbsd/openbsd_os_detector.h"

namespace EngineDoctor {

OsType OpenBsdOsDetector::Detect() {
#ifdef __OpenBSD__
    return OsType::OpenBSD;
#else
    return OsType::Unknown;
#endif
}

std::string OpenBsdOsDetector::Name() {
    return "OpenBsdOsDetector";
}

} // namespace EngineDoctor
