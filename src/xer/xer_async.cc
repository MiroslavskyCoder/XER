#include "xer/xer_async.h"

#include <uv.h>

#include <chrono>

namespace Xer {

XerAsync::XerAsync() {
    perf_counter_.Enable();
}

bool XerAsync::Sleep(const XerAsyncTask& task, std::string* error_out) const {
    if (task.timeout_ms < 0) {
        if (error_out != nullptr) {
            *error_out = "timeout_ms must be non-negative";
        }
        return false;
    }

    struct SleepState {
        bool fired = false;
    } state;

    uv_loop_t loop;
    if (uv_loop_init(&loop) != 0) {
        if (error_out != nullptr) {
            *error_out = "uv_loop_init failed";
        }
        return false;
    }

    perf_counter_.StartCounter("xer_async_sleep");
    uv_timer_t timer;
    timer.data = &state;
    uv_timer_init(&loop, &timer);
    uv_timer_start(
        &timer,
        [](uv_timer_t* handle) {
            auto* sleep_state = static_cast<SleepState*>(handle->data);
            sleep_state->fired = true;
            uv_timer_stop(handle);
            uv_close(reinterpret_cast<uv_handle_t*>(handle), nullptr);
        },
        static_cast<uint64_t>(task.timeout_ms),
        0);

    while (!state.fired) {
        uv_run(&loop, UV_RUN_DEFAULT);
    }
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
    perf_counter_.StopCounter("xer_async_sleep");

    if (error_out != nullptr) {
        error_out->clear();
    }
    return true;
}

std::int64_t XerAsync::NowMilliseconds() const {
    const auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now());
    return now.time_since_epoch().count();
}

std::string XerAsync::Report() const {
    return perf_counter_.GetReport();
}

}  // namespace Xer