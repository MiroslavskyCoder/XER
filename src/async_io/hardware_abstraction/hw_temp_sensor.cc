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
#ifdef _WIN32
    // Windows temperature reading would go here
    TemperatureReading cpu_reading;
    cpu_reading.sensor_name = "CPU";
    cpu_reading.temperature_celsius = 45.0;  // Placeholder
    cpu_reading.temperature_critical = 100.0;
    cpu_reading.is_critical = false;
    cpu_reading.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    readings_.push_back(cpu_reading);
#elif defined(__linux__)
    // Linux reads from /sys/class/thermal/
    TemperatureReading cpu_reading;
    cpu_reading.sensor_name = "CPU";
    cpu_reading.temperature_celsius = 45.0;  // Placeholder
    cpu_reading.temperature_critical = 100.0;
    cpu_reading.is_critical = false;
    cpu_reading.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    readings_.push_back(cpu_reading);
#endif
}

}  // namespace AsyncIO::IO::Hardware
