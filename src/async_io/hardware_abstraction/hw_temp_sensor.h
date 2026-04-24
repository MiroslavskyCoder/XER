#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace AsyncIO::IO::Hardware {

// Temperature sensor reading
struct TemperatureReading {
    std::string sensor_name;
    double temperature_celsius;
    double temperature_critical;
    bool is_critical;
    int64_t timestamp_ms;
};

class TemperatureSensor {
public:
    TemperatureSensor();
    ~TemperatureSensor();

    // Discovery
    bool Initialize();
    bool IsInitialized() const;
    int GetSensorCount() const;

    // Readings
    std::vector<TemperatureReading> GetAllReadings() const;
    TemperatureReading GetReading(int sensor_id) const;
    double GetMaxTemperature() const;
    double GetAverageTemperature() const;

    // CPU temperature
    double GetCPUTemperature() const;
    double GetCPUCriticalTemperature() const;

    // GPU temperature
    double GetGPUTemperature(int device_id = 0) const;
    double GetGPUCriticalTemperature(int device_id = 0) const;

    // Monitoring
    bool StartMonitoring();
    bool StopMonitoring();
    bool IsMonitoring() const { return is_monitoring_; }

    // Alerts
    void SetTemperatureAlert(double threshold_celsius);
    bool NeedsAlert() const;

private:
    std::vector<TemperatureReading> readings_;
    bool initialized_;
    bool is_monitoring_;
    double alert_threshold_;

    void UpdateReadings();
};

}  // namespace AsyncIO::IO::Hardware
