#include "threadpool.h"

#include <pthreadpool.h>
#include <stdexcept>
#include <algorithm>

namespace Engine::ML::Utils {

ThreadPool::ThreadPool(size_t num_threads) {
    // Auto-detect if num_threads = 0
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) {
            num_threads = 4;  // Fallback
        }
    }
    
    threadpool_ = pthreadpool_create(num_threads);
    if (threadpool_ == nullptr) {
        throw std::runtime_error("Failed to create pthreadpool");
    }
}

ThreadPool::~ThreadPool() {
    if (threadpool_ != nullptr) {
        pthreadpool_destroy(threadpool_);
    }
}

size_t ThreadPool::GetNumThreads() const {
    return pthreadpool_get_threads_count(threadpool_);
}

void ThreadPool::ParallelFor(std::function<void(size_t)> task, size_t num_iterations) {
    if (num_iterations == 0) {
        return;
    }
    
    // Wrapper to call std::function from C-style callback
    struct Context {
        std::function<void(size_t)> task;
    } context{task};
    
    auto wrapper = [](void* context, size_t index) {
        auto ctx = static_cast<Context*>(context);
        ctx->task(index);
    };
    
    pthreadpool_parallelize_1d(threadpool_, wrapper, &context, num_iterations);
}

void ThreadPool::ParallelForTile1D(std::function<void(size_t, size_t)> task,
                                   size_t total_size,
                                   size_t tile_size) {
    if (total_size == 0 || tile_size == 0) {
        return;
    }
    
    struct Context {
        std::function<void(size_t, size_t)> task;
    } context{task};
    
    auto wrapper = [](void* context, size_t block_start, size_t block_size) {
        auto ctx = static_cast<Context*>(context);
        ctx->task(block_start, block_size);
    };
    
    pthreadpool_parallelize_1d_tile_1d(threadpool_, wrapper, &context,
                                       total_size, tile_size);
}

void ThreadPool::ParallelFor2DTile(std::function<void(size_t, size_t, size_t, size_t)> task,
                                   size_t height, size_t width,
                                   size_t tile_h, size_t tile_w) {
    if (height == 0 || width == 0 || tile_h == 0 || tile_w == 0) {
        return;
    }
    
    struct Context {
        std::function<void(size_t, size_t, size_t, size_t)> task;
    } context{task};
    
    auto wrapper = [](void* context, size_t h_start, size_t h_size,
                      size_t w_start, size_t w_size) {
        auto ctx = static_cast<Context*>(context);
        ctx->task(h_start, h_size, w_start, w_size);
    };
    
    pthreadpool_parallelize_2d_tile_1d(threadpool_, wrapper, &context,
                                       height, width, tile_h, tile_w);
}

} // namespace Engine::ML::Utils
