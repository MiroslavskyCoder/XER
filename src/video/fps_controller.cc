#include "fps_controller.h"
#include <thread>

namespace video {

FpsController::FpsController(double target_fps) {
    SetTargetFps(target_fps);
    last_frame_time_ = std::chrono::steady_clock::now();
}

void FpsController::SetTargetFps(double fps) {
    if (fps <= 0.0) fps = 30.0;
    target_fps_    = fps;
    frame_interval_ = std::chrono::duration<double>(1.0 / fps);
}

bool FpsController::ShouldRender() {
    auto now    = std::chrono::steady_clock::now();
    auto elapsed = now - last_frame_time_;
    return elapsed >= frame_interval_;
}

void FpsController::WaitNextFrame() {
    auto deadline = last_frame_time_ + frame_interval_;
    auto now      = std::chrono::steady_clock::now();
    if (deadline > now) {
        auto remaining = deadline - now;
        if (remaining > std::chrono::milliseconds(2))
            std::this_thread::sleep_until(deadline - std::chrono::milliseconds(1));
        while (std::chrono::steady_clock::now() < deadline) {}
    }
    auto new_now  = std::chrono::steady_clock::now();
    std::chrono::duration<double> real_interval = new_now - last_frame_time_;
    actual_fps_   = (real_interval.count() > 0.0) ? 1.0 / real_interval.count() : 0.0;
    last_frame_time_ = new_now;
}

}  // namespace video