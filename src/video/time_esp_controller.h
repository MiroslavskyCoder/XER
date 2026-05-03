#pragma once
#include <cstdint>

namespace video {

/// Controls playback speed, pause, seek (Time ESP = Time Speed/Escape).
class TimeEspController {
public:
    TimeEspController() = default;

    void   SetPlaybackSpeed(double speed);  ///< 1.0 = normal, 0.5 = half, 2.0 = double
    double GetPlaybackSpeed() const { return speed_; }

    void Pause();
    void Resume();
    bool IsPaused() const { return paused_; }

    /// Seek to given PTS (microseconds).
    void SeekTo(int64_t pts_us);

    /// Advance internal clock by real_delta_us microseconds.
    /// Returns scaled delta to apply to stream PTS.
    int64_t Tick(int64_t real_delta_us);

    int64_t CurrentPts() const { return current_pts_us_; }

private:
    double  speed_{1.0};
    bool    paused_{false};
    int64_t current_pts_us_{0};
};

}  // namespace video