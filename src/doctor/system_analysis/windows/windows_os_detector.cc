#include "system_analysis/windows/windows_os_detector.h"

namespace EngineDoctor {

OsType WindowsOsDetector::Detect() {
#ifdef _WIN32
    return OsType::Windows;
#else
    return OsType::Unknown;
#endif
}

std::string WindowsOsDetector::Name() {
    return "WindowsOsDetector";
}

} // namespace EngineDoctor
