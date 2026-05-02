#pragma once
#include <string>

namespace EngineDoctor {

struct CommonSystemInfo {
    std::string hostname;
    std::string kernel_version;
    std::string arch;
    long uptime_seconds = 0;
};

class CommonSystemInfoReader {
public:
    virtual ~CommonSystemInfoReader() = default;
    virtual CommonSystemInfo Read() = 0;
};

} // namespace EngineDoctor
