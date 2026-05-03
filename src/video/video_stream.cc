#include "video_stream.h"

namespace video {

VideoStream::VideoStream(std::string id) : id_(std::move(id)) {}
VideoStream::~VideoStream() { Close(); }

void VideoStream::SetSource(std::unique_ptr<FrameSource> src) {
    source_ = std::move(src);
}

bool VideoStream::Open() {
    if (!source_) return false;
    bool ok = source_->Open();
    state_  = ok ? StreamState::Idle : StreamState::Error;
    return ok;
}

void VideoStream::Close() {
    if (source_) source_->Close();
    state_ = StreamState::Stopped;
}

void VideoStream::Play()  { state_ = StreamState::Playing; }
void VideoStream::Pause() { state_ = StreamState::Paused;  }
void VideoStream::Stop()  { state_ = StreamState::Stopped; Close(); }

std::shared_ptr<Frame> VideoStream::NextFrame() {
    if (state_ != StreamState::Playing || !source_ || !source_->IsOpen())
        return nullptr;
    if (!fps_ctrl_.ShouldRender()) return nullptr;
    fps_ctrl_.WaitNextFrame();
    return source_->NextFrame();
}

}  // namespace video