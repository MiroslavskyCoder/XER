#include "diagnostics/resource_monitor.h"
#include <filesystem>
#include <fstream>
#include <string>
namespace EngineDoctor {
ResourceStats ResourceMonitor::GetStats() {
    ResourceStats stats{};
    // count open fds
    try {
        int count = 0;
        for ([[maybe_unused]] auto& _ : std::filesystem::directory_iterator("/proc/self/fd")) count++;
        stats.fd_count = count;
    } catch (...) {}
    // read /proc/self/status
    std::ifstream f("/proc/self/status");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("Threads:", 0) == 0) stats.thread_count = std::stoi(line.substr(8));
        if (line.rfind("VmRSS:", 0) == 0) stats.mem_bytes = std::stoull(line.substr(6)) * 1024;
    }
    return stats;
}
}
