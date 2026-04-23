#include "io_thread_pool.h"

#include <cstdlib>
#include <string>

#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>

namespace IO::AsyncIO {

// Returns the thread count to use for the shared instance.
// Respects ENGINE_ASYNC_IO_WORKERS (first choice) and ENGINE_MAX_CPU_THREADS (fallback).
static size_t ResolveSharedThreadCount() {
    auto read_env_positive = [](const char* key) -> size_t {
        const char* raw = std::getenv(key);
        if (raw == nullptr || raw[0] == '\0') return 0;
        try {
            const int v = std::stoi(raw);
            return v > 0 ? static_cast<size_t>(v) : 0;
        } catch (...) {
            return 0;
        }
    };

    size_t n = read_env_positive("ENGINE_ASYNC_IO_WORKERS");
    if (n == 0) n = read_env_positive("ENGINE_MAX_CPU_THREADS");
    if (n == 0) n = std::max<size_t>(2, std::thread::hardware_concurrency());
    return n;
}

// Parse "0,1,2,3" into a cpu_set_t.  Returns false if string is empty or invalid.
static bool ParseCpuAffinity(const std::string& spec, cpu_set_t* out) {
    CPU_ZERO(out);
    if (spec.empty()) return false;
    std::string token;
    for (char c : spec + ",") {
        if (c == ',') {
            if (!token.empty()) {
                try {
                    const int core = std::stoi(token);
                    if (core >= 0 && core < CPU_SETSIZE) CPU_SET(core, out);
                } catch (...) {}
                token.clear();
            }
        } else {
            token += c;
        }
    }
    return CPU_COUNT(out) > 0;
}

// Apply affinity + nice-priority to the calling thread.
static void ApplyThreadSettings() {
    // CPU affinity
    const char* aff_env = std::getenv("ENGINE_CPU_AFFINITY");
    if (aff_env != nullptr && aff_env[0] != '\0') {
        cpu_set_t mask;
        if (ParseCpuAffinity(aff_env, &mask)) {
            pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &mask);
        }
    }

    // Thread nice-priority (relative to current).
    const char* pri_env = std::getenv("ENGINE_THREAD_PRIORITY");
    if (pri_env != nullptr && pri_env[0] != '\0') {
        try {
            const int delta = std::stoi(pri_env);
            if (delta != 0) {
                const int current = getpriority(PRIO_PROCESS, 0);
                setpriority(PRIO_PROCESS, 0, current + delta);
            }
        } catch (...) {}
    }
}

IOThreadPool& IOThreadPool::GetSharedInstance() {
    static IOThreadPool instance(ResolveSharedThreadCount());
    return instance;
}

IOThreadPool::IOThreadPool(size_t threadCount)
    : configured_thread_count_(threadCount == 0 ? 1 : threadCount) {
    Start();
}

IOThreadPool::~IOThreadPool() {
    Stop();
}

void IOThreadPool::Start() {
    std::lock_guard lock(mutex_);
    if (started_) {
        return;
    }

    stopping_ = false;
    started_ = true;
    workers_.reserve(configured_thread_count_);
    for (size_t i = 0; i < configured_thread_count_; ++i) {
        workers_.emplace_back(&IOThreadPool::WorkerLoop, this);
    }
}

void IOThreadPool::Stop() {
    {
        std::lock_guard lock(mutex_);
        if (!started_) {
            return;
        }
        stopping_ = true;
    }
    cv_.notify_all();

    for (auto &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
    {
        std::lock_guard lock(mutex_);
        started_ = false;
        while (!tasks_.empty()) {
            tasks_.pop();
        }
    }
}

void IOThreadPool::Enqueue(std::function<void()> task) {
    {
        std::lock_guard lock(mutex_);
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

size_t IOThreadPool::GetThreadCount() const {
    std::lock_guard lock(mutex_);
    return workers_.size();
}

size_t IOThreadPool::GetPendingTaskCount() const {
    std::lock_guard lock(mutex_);
    return tasks_.size();
}

bool IOThreadPool::IsRunning() const {
    std::lock_guard lock(mutex_);
    return started_ && !stopping_;
}

void IOThreadPool::WorkerLoop() {
    ApplyThreadSettings();
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
            if (stopping_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        if (task) {
            task();
        }
    }
}

} // namespace IO::AsyncIO
