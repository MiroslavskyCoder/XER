#pragma once
#include "system_analysis/base/os_detector_interface.h"
#include <memory>
#include <string>
#include <vector>

namespace EngineDoctor {

class OsDetectionManager {
public:
    void Register(std::shared_ptr<OsDetectorInterface> detector);
    OsType Detect();
    std::string DetectName();

private:
    std::vector<std::shared_ptr<OsDetectorInterface>> detectors_;
};

} // namespace EngineDoctor
