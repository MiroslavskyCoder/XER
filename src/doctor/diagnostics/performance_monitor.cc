#include "diagnostics/performance_monitor.h"
#include <chrono>
#include <fstream>
#include <thread>
namespace EngineDoctor {
static PerfSample ReadSample() {
    PerfSample s{};
    s.timestamp_ms = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            s.mem_rss_kb = std::stoul(line.substr(6));
        }
    }
    return s;
}
void PerformanceMonitor::Start(int interval_ms) {
    if (running_) return;
    running_ = true;
    thread_ = std::thread([this, interval_ms]() {
        while (running_) {
            samples_.push_back(ReadSample());
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
        }
    });
}
void PerformanceMonitor::Stop() {
    running_ = false;
    if (thread_.joinable()) thread_.join();
}
std::vector<PerfSample> PerformanceMonitor::GetSamples() const { return samples_; }
}
