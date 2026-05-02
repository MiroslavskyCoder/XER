#pragma once
#include "system_analysis/base/os_detector_interface.h"

namespace EngineDoctor {

class MacOsDetector : public OsDetectorInterface {
public:
    OsType Detect() override;
    std::string Name() override;
};

} // namespace EngineDoctor
