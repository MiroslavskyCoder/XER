#pragma once
#include <chrono>

namespace video {

/// Controls frame-rate pacing for a video stream or render loop.
class FpsController {
public:
    explicit FpsController(double target_fps = 30.0);

    void   SetTargetFps(double fps);
    double GetTargetFps() const { return target_fps_; }

    /// Call at the start of each frame. Returns true when it's time to render.
    bool   ShouldRender();

    /// Sleep until next frame deadline (busy-wait if < 1 ms remaining).
    void   WaitNextFrame();

    double ActualFps() const { return actual_fps_; }

private:
    double target_fps_{30.0};
    double actual_fps_{0.0};
    std::chrono::steady_clock::time_point last_frame_time_;
    std::chrono::duration<double> frame_interval_;
};

}  // namespace video