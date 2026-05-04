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

// Returns raw RAPL package energy in microjoules, or -1 if unavailable.
double ReadRaplEnergyUj() {
    static const std::string kRaplPath =
        "/sys/class/powercap/intel-rapl:0/energy_uj";
    std::ifstream f(kRaplPath);
    if (!f.is_open()) {
        return -1.0;
    }
    double value = 0.0;
    f >> value;
    return value;
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
    double prev_energy_uj = ReadRaplEnergyUj();

    while (running_.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));

        last_temp_.store(ReadCpuTempCelsius(), std::memory_order_relaxed);
        throttle_count_.store(ReadThrottleCount(), std::memory_order_relaxed);

        // Compute CPU package power from RAPL energy delta.
        // energy_uj wraps at max_energy_range_uj; we treat a negative delta
        // (wrap-around) as a single missed sample and skip it.
        const double cur_energy_uj = ReadRaplEnergyUj();
        if (prev_energy_uj >= 0.0 && cur_energy_uj >= 0.0) {
            const double delta_uj = cur_energy_uj - prev_energy_uj;
            if (delta_uj >= 0.0) {
                // Convert µJ over interval_ms milliseconds to Watts
                const double elapsed_s = static_cast<double>(interval_ms) * 1e-3;
                last_power_.store(delta_uj * 1e-6 / elapsed_s,
                                  std::memory_order_relaxed);
            }
        }
        prev_energy_uj = cur_energy_uj;
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
