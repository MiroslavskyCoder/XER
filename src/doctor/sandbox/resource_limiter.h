#pragma once
#include <cstddef>

namespace EngineDoctor {

class ResourceLimiter {
public:
    bool ApplyMemoryLimit(size_t max_bytes);
    bool ApplyCpuTimeLimit(int seconds);
};

}  // namespace EngineDoctor
