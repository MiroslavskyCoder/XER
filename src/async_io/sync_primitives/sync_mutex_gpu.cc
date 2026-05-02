#include "sync_mutex_gpu.h"

namespace AsyncIO::IO::Sync {

SyncMutexGpu::SyncMutexGpu(const std::string& name)
    : flag_(ATOMIC_FLAG_INIT), name_(name) {}

SyncMutexGpu::~SyncMutexGpu() = default;

void SyncMutexGpu::Lock() {
    while (flag_.test_and_set(std::memory_order_acquire)) {
        // Spin-wait
    }
}

bool SyncMutexGpu::TryLock() {
    return !flag_.test_and_set(std::memory_order_acquire);
}

void SyncMutexGpu::Unlock() {
    flag_.clear(std::memory_order_release);
}

}  // namespace AsyncIO::IO::Sync
