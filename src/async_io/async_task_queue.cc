#include "async_task_queue.h"

namespace IO::AsyncIO {

AsyncTaskQueue::AsyncTaskQueue(size_t max_queue_size)
    : max_queue_size_(max_queue_size), task_id_counter_(0), tasks_processed_(0), is_running_(true) {}

AsyncTaskQueue::~AsyncTaskQueue() {
    Shutdown();
}

uint64_t AsyncTaskQueue::EnqueueTask(AsyncTask task, int priority, CompletionCallback on_complete) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    queue_cv_.wait(lock, [this] {
        return !is_running_ || task_queue_.size() < max_queue_size_;
    });
    if (!is_running_) {
        return 0;
    }

    uint64_t task_id = ++task_id_counter_;
    QueuedTask q_task(task, on_complete, priority, task_id);
    task_queue_.push(q_task);
    
    queue_cv_.notify_one();
    return task_id;
}

bool AsyncTaskQueue::TryEnqueueTask(AsyncTask task, int priority, CompletionCallback on_complete) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    if (!is_running_ || task_queue_.size() >= max_queue_size_) {
        return false;
    }

    uint64_t task_id = ++task_id_counter_;
    QueuedTask q_task(task, on_complete, priority, task_id);
    task_queue_.push(q_task);
    
    queue_cv_.notify_one();
    return true;
}

bool AsyncTaskQueue::DequeueTask(QueuedTask& task) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    while (is_running_) {
        if (task_queue_.empty()) {
            return false;
        }

        task = task_queue_.top();
        task_queue_.pop();
        queue_cv_.notify_one();

        if (cancelled_tasks_.count(task.task_id)) {
            cancelled_tasks_.erase(task.task_id);
            continue;
        }

        ++tasks_processed_;
        return true;
    }

    return false;
}

int AsyncTaskQueue::GetQueueSize() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return static_cast<int>(task_queue_.size());
}

bool AsyncTaskQueue::IsEmpty() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return task_queue_.empty();
}

void AsyncTaskQueue::Clear() {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!task_queue_.empty()) {
        task_queue_.pop();
    }
    cancelled_tasks_.clear();
    queue_cv_.notify_all();
}

void AsyncTaskQueue::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        is_running_ = false;
        while (!task_queue_.empty()) {
            task_queue_.pop();
        }
        cancelled_tasks_.clear();
    }
    queue_cv_.notify_all();
}

bool AsyncTaskQueue::IsRunning() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return is_running_;
}

bool AsyncTaskQueue::CancelTask(uint64_t task_id) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    cancelled_tasks_.insert(task_id);
    return true;
}

size_t AsyncTaskQueue::GetCancelledCount() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return cancelled_tasks_.size();
}

uint64_t AsyncTaskQueue::GetTotalTasksProcessed() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return tasks_processed_;
}

size_t AsyncTaskQueue::GetMaxQueueSize() const {
    return max_queue_size_;
}

}  // namespace AIToolsXPro::IO::AsyncIO
