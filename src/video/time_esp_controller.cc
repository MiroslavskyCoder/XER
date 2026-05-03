#include "time_esp_controller.h"

namespace video {

void TimeEspController::SetPlaybackSpeed(double speed) {
    speed_ = (speed > 0.0) ? speed : 1.0;
}

void TimeEspController::Pause()  { paused_ = true; }
void TimeEspController::Resume() { paused_ = false; }

void TimeEspController::SeekTo(int64_t pts_us) {
    current_pts_us_ = pts_us;
}

int64_t TimeEspController::Tick(int64_t real_delta_us) {
    if (paused_) return 0;
    int64_t scaled = static_cast<int64_t>(real_delta_us * speed_);
    current_pts_us_ += scaled;
    return scaled;
}

}  // namespace video