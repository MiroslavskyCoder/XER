#pragma once

#include <atomic>
#include <string>

namespace AsyncIO::IO::Sync {

// CPU-side spinlock for use around GPU operations.
// Uses std::atomic_flag for minimal overhead.
class SyncMutexGpu {
public:
    explicit SyncMutexGpu(const std::string& name = "");
    ~SyncMutexGpu();

    void Lock();
    bool TryLock();
    void Unlock();

    const std::string& GetName() const { return name_; }

private:
    std::atomic_flag flag_;
    std::string name_;
};

}  // namespace AsyncIO::IO::Sync
