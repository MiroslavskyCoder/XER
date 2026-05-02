#include "system_analysis/openbsd/openbsd_system_info.h"
#include <sys/utsname.h>
#include <unistd.h>

namespace EngineDoctor {

CommonSystemInfo OpenBsdSystemInfo::Read() {
    CommonSystemInfo info;

    struct utsname uts;
    if (uname(&uts) == 0) {
        info.kernel_version = uts.release;
        info.arch = uts.machine;
    }

    char buf[256] = {};
    if (gethostname(buf, sizeof(buf) - 1) == 0) {
        info.hostname = buf;
    }

    return info;
}

} // namespace EngineDoctor
