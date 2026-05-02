#pragma once

#include <cstddef>
#include <cstdint>

namespace AsyncIO::IO::HW {

struct HwCapabilities {
    bool has_avx2;
    bool has_sse4;
    int  cpu_core_count;
    size_t total_ram_bytes;
    bool has_cuda;
    int  cuda_device_count;
};

class HwCapabilityScanner {
public:
    static HwCapabilityScanner& Instance();

    HwCapabilities Scan();

    HwCapabilityScanner(const HwCapabilityScanner&) = delete;
    HwCapabilityScanner& operator=(const HwCapabilityScanner&) = delete;

private:
    HwCapabilityScanner() = default;
    ~HwCapabilityScanner() = default;
};

}  // namespace AsyncIO::IO::HW
