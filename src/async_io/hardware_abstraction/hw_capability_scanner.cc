#include "hw_capability_scanner.h"

#include <thread>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#  include <cpuid.h>
#endif

#if defined(__linux__)
#  include <sys/sysinfo.h>
#endif

namespace AsyncIO::IO::HW {

HwCapabilityScanner& HwCapabilityScanner::Instance() {
    static HwCapabilityScanner instance;
    return instance;
}

HwCapabilities HwCapabilityScanner::Scan() {
    HwCapabilities caps{};

    // CPU core count
    caps.cpu_core_count = static_cast<int>(std::thread::hardware_concurrency());

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
    // SSE4 / AVX2 detection via CPUID
    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        caps.has_sse4 = (ecx & bit_SSE4_2) != 0;
    }
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        caps.has_avx2 = (ebx & bit_AVX2) != 0;
    }
#endif

    // Total RAM
#if defined(__linux__)
    struct sysinfo si{};
    if (sysinfo(&si) == 0) {
        caps.total_ram_bytes = static_cast<size_t>(si.totalram) * si.mem_unit;
    }
#endif

    // CUDA — stub; real detection requires libcuda or nvml
    caps.has_cuda = false;
    caps.cuda_device_count = 0;

    return caps;
}

}  // namespace AsyncIO::IO::HW
