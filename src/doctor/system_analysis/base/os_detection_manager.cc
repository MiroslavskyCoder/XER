#include "system_analysis/base/os_detection_manager.h"

namespace EngineDoctor {

void OsDetectionManager::Register(std::shared_ptr<OsDetectorInterface> detector) {
    detectors_.push_back(std::move(detector));
}

OsType OsDetectionManager::Detect() {
    for (auto& d : detectors_) {
        OsType t = d->Detect();
        if (t != OsType::Unknown) return t;
    }
    return OsType::Unknown;
}

std::string OsDetectionManager::DetectName() {
    for (auto& d : detectors_) {
        OsType t = d->Detect();
        if (t != OsType::Unknown) return d->Name();
    }
    return "Unknown";
}

} // namespace EngineDoctor
