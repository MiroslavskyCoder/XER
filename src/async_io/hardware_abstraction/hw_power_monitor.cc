#include "hw_power_monitor.h"

#include <fstream>
#include <string>

namespace AsyncIO::IO::HW {

namespace {

double ReadSysfsDouble(const std::string& path, double scale = 1.0) {
    std::ifstream f(path);
    if (!f.is_open()) {
        return 0.0;
    }
    double value = 0.0;
    f >> value;
    return value * scale;
}

double ReadCpuTempCelsius() {
    // /sys/class/thermal/thermal_zone0/temp reports millidegrees Celsius
    return ReadSysfsDouble("/sys/class/thermal/thermal_zone0/temp", 1e-3);
}

double ReadCpuPowerWatts() {
    // RAPL energy counter at /sys/class/powercap/intel-rapl:0/energy_uj (microjoules)
    // Power is not directly readable without two samples; return 0 as stub.
    return 0.0;
}

int ReadThrottleCount() {
    // /sys/devices/system/cpu/cpu0/thermal_throttle/core_throttle_count
    double val = ReadSysfsDouble(
        "/sys/devices/system/cpu/cpu0/thermal_throttle/core_throttle_count");
    return static_cast<int>(val);
}

}  // namespace

HwPowerMonitor::HwPowerMonitor()
    : running_(false), last_temp_(0.0), last_power_(0.0), throttle_count_(0) {}

HwPowerMonitor::~HwPowerMonitor() {
    Stop();
}

void HwPowerMonitor::Start(int interval_ms) {
    if (running_.exchange(true)) {
        return;  // Already running
    }
    monitor_thread_ = std::thread(&HwPowerMonitor::MonitorLoop, this, interval_ms);
}

void HwPowerMonitor::Stop() {
    if (!running_.exchange(false)) {
        return;
    }
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
}

void HwPowerMonitor::MonitorLoop(int interval_ms) {
    while (running_.load(std::memory_order_relaxed)) {
        last_temp_.store(ReadCpuTempCelsius(), std::memory_order_relaxed);
        last_power_.store(ReadCpuPowerWatts(), std::memory_order_relaxed);
        throttle_count_.store(ReadThrottleCount(), std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }
}

PowerStats HwPowerMonitor::GetStats() const {
    return PowerStats{
        last_temp_.load(std::memory_order_relaxed),
        last_power_.load(std::memory_order_relaxed),
        throttle_count_.load(std::memory_order_relaxed),
    };
}

}  // namespace AsyncIO::IO::HW
