#include "render_target.h"

namespace video {

RenderTarget::RenderTarget(int width, int height, PixelFormat fmt) {
    Resize(width, height, fmt);
}

void RenderTarget::Resize(int width, int height, PixelFormat fmt) {
    width_  = width;
    height_ = height;
    fmt_    = fmt;
    frame_  = std::make_unique<Frame>();
    frame_->Allocate(width, height, fmt);
}

}  // namespace video