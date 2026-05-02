#include "system_analysis/windows/windows_system_info.h"

namespace EngineDoctor {

CommonSystemInfo WindowsSystemInfo::Read() {
    CommonSystemInfo info;
    info.arch = "x86_64";
    return info;
}

} // namespace EngineDoctor
