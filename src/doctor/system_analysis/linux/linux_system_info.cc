#include "system_analysis/linux/linux_system_info.h"
#include <cstring>
#include <fstream>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

namespace EngineDoctor {

CommonSystemInfo LinuxSystemInfo::Read() {
    CommonSystemInfo info;

    // kernel_version from /proc/version
    {
        std::ifstream f("/proc/version");
        if (f.is_open()) {
            std::getline(f, info.kernel_version);
        }
    }

    // hostname via gethostname
    {
        char buf[256] = {};
        if (gethostname(buf, sizeof(buf) - 1) == 0) {
            info.hostname = buf;
        }
    }

    // arch via uname
    {
        struct utsname uts;
        if (uname(&uts) == 0) {
            info.arch = uts.machine;
        }
    }

    // uptime via sysinfo
    {
        struct sysinfo si;
        if (sysinfo(&si) == 0) {
            info.uptime_seconds = static_cast<long>(si.uptime);
        }
    }

    return info;
}

} // namespace EngineDoctor
