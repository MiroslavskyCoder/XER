#pragma once
#include "frame.h"
#include <memory>

namespace video {

/// Surface that a renderer writes output into.
class RenderTarget {
public:
    RenderTarget() = default;
    explicit RenderTarget(int width, int height, PixelFormat fmt = PixelFormat::BGRA8);

    void Resize(int width, int height, PixelFormat fmt = PixelFormat::BGRA8);

    int          Width()  const { return width_; }
    int          Height() const { return height_; }
    PixelFormat  Format() const { return fmt_; }
    bool         IsReady() const { return frame_ != nullptr; }

    Frame*       GetFrame() { return frame_.get(); }
    const Frame* GetFrame() const { return frame_.get(); }

private:
    int width_{0}, height_{0};
    PixelFormat fmt_{PixelFormat::BGRA8};
    std::unique_ptr<Frame> frame_;
};

}  // namespace video