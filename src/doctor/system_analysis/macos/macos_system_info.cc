#include "system_analysis/macos/macos_system_info.h"
#include <cstdio>
#include <string>
#include <unistd.h>

namespace EngineDoctor {

CommonSystemInfo MacOsSystemInfo::Read() {
    CommonSystemInfo info;

    // kernel_version via uname -r
    {
        FILE* pipe = popen("uname -r 2>/dev/null", "r");
        if (pipe) {
            char buf[256] = {};
            if (fgets(buf, sizeof(buf) - 1, pipe)) {
                std::string s(buf);
                if (!s.empty() && s.back() == '\n') s.pop_back();
                info.kernel_version = s;
            }
            pclose(pipe);
        }
    }

    // hostname via gethostname
    {
        char buf[256] = {};
        if (gethostname(buf, sizeof(buf) - 1) == 0) {
            info.hostname = buf;
        }
    }

    return info;
}

} // namespace EngineDoctor
