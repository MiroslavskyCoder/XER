#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace AsyncIO::IO::Hardware {

struct ProcessorCore {
    int core_id;
    int logical_id;
    uint64_t frequency_mhz;
    bool is_active;
};

struct CPUInfo {
    std::string vendor;
    std::string model;
    int physical_cores;
    int logical_cores;
    uint64_t l1_cache_kb;
    uint64_t l2_cache_kb;
    uint64_t l3_cache_kb;
    bool supports_sse;
    bool supports_avx;
    bool supports_avx2;
    bool supports_avx512;
};

class CPUInfoProvider {
public:
    CPUInfoProvider();
    ~CPUInfoProvider();

    // CPU detection
    bool Initialize();
    bool IsInitialized() const;

    // Information
    const CPUInfo& GetInfo() const { return cpu_info_; }
    std::vector<ProcessorCore> GetCores() const;
    uint64_t GetTotalMemoryMB() const;
    uint64_t GetAvailableMemoryMB() const;

    // Core affinity
    bool SetThreadAffinity(int core_id);
    int GetCurrentCore() const;

    // Performance monitoring
    double GetCPUUsagePercent() const;
    double GetCoreUsagePercent(int core_id) const;

    // Info string
    std::string GetCPUDescription() const;

private:
    CPUInfo cpu_info_;
    std::vector<ProcessorCore> cores_;
    bool initialized_;
    
    void DetectCPUCapabilities();
    void DetectCoreCount();
};

}  // namespace AsyncIO::IO::Hardware
