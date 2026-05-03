#include "frame.h"

namespace video {

Frame::Frame(int w, int h, PixelFormat fmt) {
    Allocate(w, h, fmt);
}

bool Frame::Allocate(int w, int h, PixelFormat fmt) {
    meta_.width  = w;
    meta_.height = h;
    meta_.format = fmt;
    size_t sz = 0;
    switch (fmt) {
        case PixelFormat::BGRA8:
        case PixelFormat::RGBA8: sz = static_cast<size_t>(w) * h * 4; break;
        case PixelFormat::NV12:  sz = static_cast<size_t>(w) * h * 3 / 2; break;
        case PixelFormat::YUV420P: sz = static_cast<size_t>(w) * h * 3 / 2; break;
    }
    data_.assign(sz, 0);
    return sz > 0;
}

void Frame::Release() {
    data_.clear();
    meta_ = {};
}

int Frame::Stride() const {
    switch (meta_.format) {
        case PixelFormat::BGRA8:
        case PixelFormat::RGBA8: return meta_.width * 4;
        case PixelFormat::NV12:
        case PixelFormat::YUV420P: return meta_.width;
    }
    return meta_.width;
}

}  // namespace video