#pragma once
#include "frame.h"
#include "frame_source.h"
#include "fps_controller.h"
#include "video_constants.h"
#include <memory>
#include <string>
#include <atomic>

namespace video {

enum class StreamState : uint8_t { Idle, Playing, Paused, Stopped, Error };

/// Represents a single video stream with its own FPS controller and state.
class VideoStream {
public:
    explicit VideoStream(std::string  id);
    ~VideoStream();

    const std::string& Id() const { return id_; }
    StreamState        State() const { return state_; }

    void SetSource(std::unique_ptr<FrameSource> src);
    bool Open();

    /// Convenience: set URI, auto-create FFmpegFrameSource if none set, then Open().
    bool OpenUri(const std::string& uri);
    const std::string& Uri() const { return uri_; }

    void Close();
    void Play();
    void Pause();
    void Stop();

    /// Fetch next frame (returns nullptr if not available).
    std::shared_ptr<Frame> NextFrame();

    void   SetTargetFps(double fps) { fps_ctrl_.SetTargetFps(fps); }
    double GetActualFps() const     { return fps_ctrl_.ActualFps(); }

private:
    std::string                    id_;
    std::string                    uri_;
    std::atomic<StreamState>       state_{StreamState::Idle};
    std::unique_ptr<FrameSource>   source_;
    FpsController                  fps_ctrl_;
};

}  // namespace video