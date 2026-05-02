#pragma once

#include <condition_variable>
#include <mutex>
#include <string>

namespace AsyncIO::IO::Sync {

class SyncSemaphoreCpu {
public:
    explicit SyncSemaphoreCpu(int initial_count, const std::string& name = "");
    ~SyncSemaphoreCpu();

    void Acquire();
    bool TryAcquire();
    void Release();

    int GetCount() const;
    const std::string& GetName() const { return name_; }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    int count_;
    std::string name_;
};

}  // namespace AsyncIO::IO::Sync
