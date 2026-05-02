#include "system_analysis/macos/macos_os_detector.h"

namespace EngineDoctor {

OsType MacOsDetector::Detect() {
#ifdef __APPLE__
    return OsType::MacOS;
#else
    return OsType::Unknown;
#endif
}

std::string MacOsDetector::Name() {
    return "MacOsDetector";
}

} // namespace EngineDoctor
