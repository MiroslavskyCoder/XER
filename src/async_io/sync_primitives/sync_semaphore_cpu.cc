#include "sync_semaphore_cpu.h"

#include <stdexcept>

namespace AsyncIO::IO::Sync {

SyncSemaphoreCpu::SyncSemaphoreCpu(int initial_count, const std::string& name)
    : count_(initial_count), name_(name) {
    if (initial_count < 0) {
        throw std::invalid_argument("SyncSemaphoreCpu: initial_count must be >= 0");
    }
}

SyncSemaphoreCpu::~SyncSemaphoreCpu() = default;

void SyncSemaphoreCpu::Acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return count_ > 0; });
    --count_;
}

bool SyncSemaphoreCpu::TryAcquire() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (count_ > 0) {
        --count_;
        return true;
    }
    return false;
}

void SyncSemaphoreCpu::Release() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++count_;
    }
    cv_.notify_one();
}

int SyncSemaphoreCpu::GetCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return count_;
}

}  // namespace AsyncIO::IO::Sync
