#pragma once
#include <cstddef>
namespace EngineDoctor {
struct ResourceStats { int fd_count; int thread_count; size_t mem_bytes; };
class ResourceMonitor {
public:
    ResourceStats GetStats();
};
}
