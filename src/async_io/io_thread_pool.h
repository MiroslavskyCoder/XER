#ifndef IO_ASYNC_IO_THREAD_POOL_H
#define IO_ASYNC_IO_THREAD_POOL_H

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace IO::AsyncIO {

class IOThreadPool {
public:
    explicit IOThreadPool(size_t threadCount = std::thread::hardware_concurrency());
    ~IOThreadPool();

    static IOThreadPool& GetSharedInstance();

    IOThreadPool(const IOThreadPool &) = delete;
    IOThreadPool &operator=(const IOThreadPool &) = delete;

    void Start();
    void Stop();
    void Enqueue(std::function<void()> task);
    size_t GetThreadCount() const;
    size_t GetPendingTaskCount() const;
    bool IsRunning() const;

private:
    void WorkerLoop();

    size_t configured_thread_count_;
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool stopping_ = false;
    bool started_ = false;
};

} // namespace IO::AsyncIO

#endif // IO_ASYNC_IO_THREAD_POOL_H
