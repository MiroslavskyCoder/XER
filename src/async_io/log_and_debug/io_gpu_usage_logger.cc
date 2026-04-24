#include "io_gpu_usage_logger.h"
  
#include <cuda_runtime.h> 

namespace AsyncIO::IO::LogDebug {

GPUUsageLogger::GPUUsageLogger()
    : initialized_(false), is_logging_(false) {}

GPUUsageLogger::~GPUUsageLogger() {
    StopLogging();
}

bool GPUUsageLogger::Initialize() { 
    int device_count = 0;
    if (cudaGetDeviceCount(&device_count) == cudaSuccess && device_count > 0) {
        initialized_ = true;
        return true;
    } 
}

bool GPUUsageLogger::IsInitialized() const {
    return initialized_;
}

void GPUUsageLogger::StartLogging() {
    is_logging_ = true;
}

void GPUUsageLogger::StopLogging() {
    is_logging_ = false;
}

bool GPUUsageLogger::CaptureSnapshot(int device_id) {
    if (!initialized_) return false;

    GPUUsageSnapshot snapshot;
    if (QueryGPUUsage(device_id, snapshot)) {
        snapshots_.push_back(snapshot);
        return true;
    }
    
    return false;
}

std::vector<GPUUsageSnapshot> GPUUsageLogger::GetSnapshots(int device_id) const {
    std::vector<GPUUsageSnapshot> filtered;
    for (const auto& snap : snapshots_) {
        if (snap.device_id == device_id) {
            filtered.push_back(snap);
        }
    }
    return filtered;
}

double GPUUsageLogger::GetAverageUtilization(int device_id) const {
    auto snaps = GetSnapshots(device_id);
    if (snaps.empty()) return 0.0;

    double sum = 0.0;
    for (const auto& snap : snaps) {
        sum += snap.utilization_percent;
    }
    return sum / snaps.size();
}

double GPUUsageLogger::GetMaxUtilization(int device_id) const {
    auto snaps = GetSnapshots(device_id);
    double max_util = 0.0;
    
    for (const auto& snap : snaps) {
        if (snap.utilization_percent > max_util) {
            max_util = snap.utilization_percent;
        }
    }
    
    return max_util;
}

double GPUUsageLogger::GetAverageTemperature(int device_id) const {
    auto snaps = GetSnapshots(device_id);
    if (snaps.empty()) return 0.0;

    double sum = 0.0;
    for (const auto& snap : snaps) {
        sum += snap.temperature_celsius;
    }
    return sum / snaps.size();
}

double GPUUsageLogger::GetPeakMemoryUsage(int device_id) const {
    auto snaps = GetSnapshots(device_id);
    double peak = 0.0;
    
    for (const auto& snap : snaps) {
        if (snap.memory_used_mb > peak) {
            peak = snap.memory_used_mb;
        }
    }
    
    return peak;
}

std::string GPUUsageLogger::GetUsageReport(int device_id) const {
    std::string report;
    report += "=== GPU Usage Report (Device " + std::to_string(device_id) + ") ===\n";
    report += "Average Utilization: " + std::to_string(GetAverageUtilization(device_id)) + "%\n";
    report += "Peak Utilization: " + std::to_string(GetMaxUtilization(device_id)) + "%\n";
    report += "Average Temperature: " + std::to_string(GetAverageTemperature(device_id)) + "C\n";
    report += "Peak Memory Usage: " + std::to_string(GetPeakMemoryUsage(device_id)) + "MB\n";
    
    return report;
}

void GPUUsageLogger::ClearSnapshots() {
    snapshots_.clear();
}

bool GPUUsageLogger::QueryGPUUsage(int device_id, GPUUsageSnapshot& snapshot) { 
    snapshot.device_id = device_id;
    snapshot.utilization_percent = 0.0;  // Would use NVML in production
    snapshot.memory_used_mb = 0.0;
    snapshot.memory_free_mb = 0.0;
    snapshot.temperature_celsius = 0.0;
    snapshot.power_draw_watts = 0.0;
    snapshot.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    return true; 
}

}  // namespace AsyncIO::IO::LogDebug
