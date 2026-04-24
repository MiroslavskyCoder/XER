#include "cpu_info.h"

#if defined(_WIN32)
    #include <windows.h>
    #include <intrin.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <cpuid.h>
#endif

namespace AsyncIO::IO::Hardware {

CPUInfoProvider::CPUInfoProvider()
    : initialized_(false) {
    cpu_info_ = CPUInfo();
}

CPUInfoProvider::~CPUInfoProvider() {}

bool CPUInfoProvider::Initialize() {
#ifdef _WIN32
    // Windows CPU detection
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    
    cpu_info_.logical_cores = sys_info.dwNumberOfProcessors;
    cpu_info_.physical_cores = sys_info.dwNumberOfProcessors / 2;  // Simplified
    
    DetectCPUCapabilities();
    DetectCoreCount();
    
    initialized_ = true;
    return true;
#elif defined(__linux__)
    // Linux CPU detection
    cpu_info_.logical_cores = sysconf(_SC_NPROCESSORS_ONLN);
    cpu_info_.physical_cores = cpu_info_.logical_cores / 2;  // Simplified
    
    DetectCPUCapabilities();
    DetectCoreCount();
    
    initialized_ = true;
    return true;
#else
    return false;
#endif
}

bool CPUInfoProvider::IsInitialized() const {
    return initialized_;
}

std::vector<ProcessorCore> CPUInfoProvider::GetCores() const {
    return cores_;
}

uint64_t CPUInfoProvider::GetTotalMemoryMB() const {
#ifdef _WIN32
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    GlobalMemoryStatusEx(&mem_status);
    return mem_status.ullTotalPhys / (1024 * 1024);
#elif defined(__linux__)
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return (pages * page_size) / (1024 * 1024);
#else
    return 0;
#endif
}

uint64_t CPUInfoProvider::GetAvailableMemoryMB() const {
#ifdef _WIN32
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    GlobalMemoryStatusEx(&mem_status);
    return mem_status.ullAvailPhys / (1024 * 1024);
#elif defined(__linux__)
    long avail_pages = sysconf(_SC_AVPHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return (avail_pages * page_size) / (1024 * 1024);
#else
    return 0;
#endif
}

bool CPUInfoProvider::SetThreadAffinity(int core_id) {
#ifdef _WIN32
    HANDLE current_thread = GetCurrentThread();
    DWORD_PTR mask = 1ULL << core_id;
    return (SetThreadAffinityMask(current_thread, mask) != 0);
#else
    return false;  // Linux implementation would use pthread_setaffinity_np
#endif
}

int CPUInfoProvider::GetCurrentCore() const {
#ifdef _WIN32
    return GetCurrentProcessorNumber();
#else
    return 0;  // Linux implementation would use sched_getcpu()
#endif
}

double CPUInfoProvider::GetCPUUsagePercent() const {
    // Placeholder - would require system-specific implementation
    return 0.0;
}

double CPUInfoProvider::GetCoreUsagePercent(int core_id) const {
    // Placeholder - would require system-specific implementation
    return 0.0;
}

std::string CPUInfoProvider::GetCPUDescription() const {
    std::string desc = cpu_info_.vendor + " " + cpu_info_.model;
    desc += " (" + std::to_string(cpu_info_.physical_cores) + "P/" + 
            std::to_string(cpu_info_.logical_cores) + "L)";
    return desc;
}

void CPUInfoProvider::DetectCPUCapabilities() {
#ifdef _WIN32
    int regs[4];
    __cpuid(regs, 1);
    
    // ECX register bits for SSE, AVX detection
    cpu_info_.supports_sse = (regs[2] & 0x1000000) != 0;   // SSE2 in EDX
    cpu_info_.supports_avx = (regs[2] & 0x10000000) != 0;  // AVX in ECX
    
    __cpuid(regs, 7);
    cpu_info_.supports_avx2 = (regs[1] & 0x20) != 0;       // AVX2 in EBX
    cpu_info_.supports_avx512 = (regs[1] & 0x10000) != 0;  // AVX-512F in EBX
#endif
}

void CPUInfoProvider::DetectCoreCount() {
    for (int i = 0; i < cpu_info_.logical_cores; ++i) {
        ProcessorCore core;
        core.core_id = i / 2;
        core.logical_id = i;
        core.frequency_mhz = 0;
        core.is_active = true;
        cores_.push_back(core);
    }
}

}  // namespace AsyncIO::IO::Hardware
