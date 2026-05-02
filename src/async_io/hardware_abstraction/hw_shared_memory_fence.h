#pragma once

#include <atomic>
#include <string>

namespace AsyncIO::IO::HW {

class HwSharedMemoryFence {
public:
    explicit HwSharedMemoryFence(const std::string& name = "");
    ~HwSharedMemoryFence();

    void Acquire();
    void Release();
    bool TryAcquire();

    const std::string& GetName() const { return name_; }

private:
    std::atomic_flag atomic_flag_;
    std::string name_;
};

}  // namespace AsyncIO::IO::HW
