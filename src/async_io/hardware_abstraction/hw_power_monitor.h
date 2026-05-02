#pragma once

#include <atomic>
#include <chrono>
#include <thread>

namespace AsyncIO::IO::HW {

struct PowerStats {
    double cpu_temp_celsius;
    double cpu_power_watts;
    int    throttle_count;
};

class HwPowerMonitor {
public:
    HwPowerMonitor();
    ~HwPowerMonitor();

    void Start(int interval_ms);
    void Stop();

    PowerStats GetStats() const;
    bool IsRunning() const { return running_.load(std::memory_order_relaxed); }

private:
    void MonitorLoop(int interval_ms);

    std::atomic<bool> running_;
    std::thread monitor_thread_;

    // Cached last-read values (written by monitor thread, read by GetStats)
    mutable std::atomic<double> last_temp_;
    mutable std::atomic<double> last_power_;
    std::atomic<int>            throttle_count_;
};

}  // namespace AsyncIO::IO::HW
