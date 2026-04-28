#include "mutex_wrapper.h"

namespace AsyncIO::IO::Sync {

MutexWrapper::MutexWrapper(const std::string& name)
    : name_(name), lock_count_(0), contention_count_(0) {}

MutexWrapper::~MutexWrapper() {}

void MutexWrapper::Lock() {
    if (!TryLock()) {
        contention_count_++;
        mutex_.lock();
    }
    lock_count_++;
}

bool MutexWrapper::TryLock() {
    bool success = mutex_.try_lock();
    if (success) {
        lock_count_++;
    }
    return success;
}

bool MutexWrapper::TryLockFor(const std::chrono::milliseconds& timeout) {
    bool success = false;
    
    auto timed_mutex_ptr = dynamic_cast<std::timed_mutex*>(&mutex_);
    if (timed_mutex_ptr) {
        success = timed_mutex_ptr->try_lock_for(timeout);
    } else {
        success = TryLock();
    }
    
    if (success) {
        lock_count_++;
    } else {
        contention_count_++;
    }
    
    return success;
}

void MutexWrapper::Unlock() {
    mutex_.unlock();
}

}  // namespace AsyncIO::IO::Sync
