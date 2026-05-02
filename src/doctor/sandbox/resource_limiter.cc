#include "sandbox/resource_limiter.h"
#include <sys/resource.h>

namespace EngineDoctor {

bool ResourceLimiter::ApplyMemoryLimit(size_t max_bytes) {
    struct rlimit rl{};
    rl.rlim_cur = static_cast<rlim_t>(max_bytes);
    rl.rlim_max = static_cast<rlim_t>(max_bytes);
    return setrlimit(RLIMIT_AS, &rl) == 0;
}

bool ResourceLimiter::ApplyCpuTimeLimit(int seconds) {
    struct rlimit rl{};
    rl.rlim_cur = static_cast<rlim_t>(seconds);
    rl.rlim_max = static_cast<rlim_t>(seconds);
    return setrlimit(RLIMIT_CPU, &rl) == 0;
}

}  // namespace EngineDoctor
