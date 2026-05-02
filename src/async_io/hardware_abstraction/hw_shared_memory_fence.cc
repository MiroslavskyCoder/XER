#include "hw_shared_memory_fence.h"

namespace AsyncIO::IO::HW {

HwSharedMemoryFence::HwSharedMemoryFence(const std::string& name)
    : atomic_flag_(ATOMIC_FLAG_INIT), name_(name) {}

HwSharedMemoryFence::~HwSharedMemoryFence() = default;

void HwSharedMemoryFence::Acquire() {
    while (atomic_flag_.test_and_set(std::memory_order_acquire)) {
        // Spin-wait
    }
}

void HwSharedMemoryFence::Release() {
    atomic_flag_.clear(std::memory_order_release);
}

bool HwSharedMemoryFence::TryAcquire() {
    return !atomic_flag_.test_and_set(std::memory_order_acquire);
}

}  // namespace AsyncIO::IO::HW
