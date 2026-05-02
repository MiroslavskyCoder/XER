#pragma once
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>
namespace EngineDoctor {
struct PerfSample { double cpu_percent; size_t mem_rss_kb; uint64_t timestamp_ms; };
class PerformanceMonitor {
public:
    void Start(int interval_ms);
    void Stop();
    std::vector<PerfSample> GetSamples() const;
private:
    std::vector<PerfSample> samples_;
    bool running_ = false;
    std::thread thread_;
};
}
