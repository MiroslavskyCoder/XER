#pragma once

#include <string>
#include <map>
#include <cstdint>
#include <chrono>
#include <mutex>
#include <vector>

namespace AsyncIO::IO::LogDebug {

struct PerformanceMetric {
    std::string name;
    uint64_t count;
    double total_ms;
    double min_ms;
    double max_ms;
    double avg_ms;
    int64_t last_measured_ms;
};

class PerformanceCounter {
public:
    PerformanceCounter();
    ~PerformanceCounter();

    // Counter management
    void StartCounter(const std::string& name);
    void StopCounter(const std::string& name);
    void ResetCounter(const std::string& name);
    void ResetAll();

    // Metrics retrieval
    PerformanceMetric GetMetric(const std::string& name) const;
    std::vector<PerformanceMetric> GetAllMetrics() const;
    
    // Timing helpers
    double GetTotalTimeMS(const std::string& name) const;
    double GetAverageTimeMS(const std::string& name) const;
    uint64_t GetCallCount(const std::string& name) const;

    // Reports
    std::string GetReport() const;
    std::string GetMetricReport(const std::string& name) const;

    // Enable/disable
    void Enable() { enabled_ = true; }
    void Disable() { enabled_ = false; }
    bool IsEnabled() const { return enabled_; }

private:
    mutable std::mutex mutex_;
    std::map<std::string, PerformanceMetric> metrics_;
    std::map<std::string, std::chrono::steady_clock::time_point> active_counters_;
    bool enabled_;

    void UpdateMetric(const std::string& name, double elapsed_ms);
};

}  // namespace AsyncIO::IO::LogDebug
