#include "hw_temp_sensor.h"

#ifdef _WIN32
    #include <windows.h>
#elif defined(__linux__)
    #include <fstream>
#endif

#include <chrono>
#include <algorithm>

namespace AsyncIO::IO::Hardware {

TemperatureSensor::TemperatureSensor()
    : initialized_(false), is_monitoring_(false), alert_threshold_(90.0) {}

TemperatureSensor::~TemperatureSensor() {
    StopMonitoring();
}

bool TemperatureSensor::Initialize() {
    UpdateReadings();
    initialized_ = true;
    return readings_.size() > 0;
}

bool TemperatureSensor::IsInitialized() const {
    return initialized_;
}

int TemperatureSensor::GetSensorCount() const {
    return static_cast<int>(readings_.size());
}

std::vector<TemperatureReading> TemperatureSensor::GetAllReadings() const {
    return readings_;
}

TemperatureReading TemperatureSensor::GetReading(int sensor_id) const {
    if (sensor_id >= 0 && sensor_id < static_cast<int>(readings_.size())) {
        return readings_[sensor_id];
    }
    return TemperatureReading();
}

double TemperatureSensor::GetMaxTemperature() const {
    double max_temp = 0.0;
    for (const auto& reading : readings_) {
        if (reading.temperature_celsius > max_temp) {
            max_temp = reading.temperature_celsius;
        }
    }
    return max_temp;
}

double TemperatureSensor::GetAverageTemperature() const {
    if (readings_.empty()) return 0.0;
    
    double sum = 0.0;
    for (const auto& reading : readings_) {
        sum += reading.temperature_celsius;
    }
    return sum / readings_.size();
}

double TemperatureSensor::GetCPUTemperature() const {
    for (const auto& reading : readings_) {
        if (reading.sensor_name.find("CPU") != std::string::npos) {
            return reading.temperature_celsius;
        }
    }
    return 0.0;
}

double TemperatureSensor::GetCPUCriticalTemperature() const {
    for (const auto& reading : readings_) {
        if (reading.sensor_name.find("CPU") != std::string::npos) {
            return reading.temperature_critical;
        }
    }
    return 100.0;
}

double TemperatureSensor::GetGPUTemperature(int device_id) const {
    std::string pattern = "GPU" + std::to_string(device_id);
    for (const auto& reading : readings_) {
        if (reading.sensor_name.find(pattern) != std::string::npos) {
            return reading.temperature_celsius;
        }
    }
    return 0.0;
}

double TemperatureSensor::GetGPUCriticalTemperature(int device_id) const {
    std::string pattern = "GPU" + std::to_string(device_id);
    for (const auto& reading : readings_) {
        if (reading.sensor_name.find(pattern) != std::string::npos) {
            return reading.temperature_critical;
        }
    }
    return 100.0;
}

bool TemperatureSensor::StartMonitoring() {
    is_monitoring_ = true;
    return true;
}

bool TemperatureSensor::StopMonitoring() {
    is_monitoring_ = false;
    return true;
}

void TemperatureSensor::SetTemperatureAlert(double threshold_celsius) {
    alert_threshold_ = threshold_celsius;
}

bool TemperatureSensor::NeedsAlert() const {
    return GetMaxTemperature() >= alert_threshold_;
}

void TemperatureSensor::UpdateReadings() {
    readings_.clear();
    const int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();

#ifdef __linux__
    // Enumerate /sys/class/thermal/thermal_zoneN
    for (int zone = 0; zone < 32; ++zone) {
        const std::string base = "/sys/class/thermal/thermal_zone" + std::to_string(zone);

        std::ifstream type_f(base + "/type");
        if (!type_f.is_open()) break;  // no more zones

        std::string sensor_type;
        std::getline(type_f, sensor_type);

        std::ifstream temp_f(base + "/temp");
        if (!temp_f.is_open()) continue;
        int64_t raw_millideg = 0;
        temp_f >> raw_millideg;
        const double celsius = static_cast<double>(raw_millideg) / 1000.0;

        // Try to read trip_point_X_temp for critical threshold
        double critical = 100.0;
        for (int tp = 0; tp < 8; ++tp) {
            std::ifstream tt(base + "/trip_point_" + std::to_string(tp) + "_type");
            if (!tt.is_open()) break;
            std::string ttype;
            std::getline(tt, ttype);
            if (ttype == "critical") {
                std::ifstream tv(base + "/trip_point_" + std::to_string(tp) + "_temp");
                if (tv.is_open()) {
                    int64_t crit_raw = 0;
                    tv >> crit_raw;
                    critical = static_cast<double>(crit_raw) / 1000.0;
                }
                break;
            }
        }

        TemperatureReading r;
        r.sensor_name = sensor_type;
        r.temperature_celsius = celsius;
        r.temperature_critical = critical;
        r.is_critical = celsius >= critical;
        r.timestamp_ms = now_ms;
        readings_.push_back(r);
    }

    // Fallback: if no zones found, add a synthetic CPU entry
    if (readings_.empty()) {
        TemperatureReading r;
        r.sensor_name = "CPU";
        r.temperature_celsius = 0.0;
        r.temperature_critical = 100.0;
        r.is_critical = false;
        r.timestamp_ms = now_ms;
        readings_.push_back(r);
    }
#elif defined(_WIN32)
    // Windows: add a placeholder entry; WMI-based reading is out of scope
    TemperatureReading r;
    r.sensor_name = "CPU";
    r.temperature_celsius = 0.0;
    r.temperature_critical = 100.0;
    r.is_critical = false;
    r.timestamp_ms = now_ms;
    readings_.push_back(r);
#endif
}

}  // namespace AsyncIO::IO::Hardware
