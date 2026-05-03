#include "image_buffer.h"
#include <cstring>
#include <algorithm>

namespace image {

ImageBuffer::ImageBuffer(int w, int h, PixelFormat fmt, ColorSpace cs)
    : width_(w), height_(h), fmt_(fmt), cs_(cs) {
    Allocate(w, h, fmt, cs);
}

void ImageBuffer::Allocate(int w, int h, PixelFormat fmt, ColorSpace cs) {
    width_  = w;
    height_ = h;
    fmt_    = fmt;
    cs_     = cs;
    int bpp = BytesPerPixel(fmt);
    if (bpp > 0) {
        data_.assign(static_cast<size_t>(w) * h * bpp, 0);
    } else if (fmt == PixelFormat::RGBA32F) {
        float_data_.assign(static_cast<size_t>(w) * h * 4, 0.0f);
    }
}

void ImageBuffer::Clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (fmt_ == PixelFormat::RGBA8 || fmt_ == PixelFormat::BGRA8) {
        for (int i = 0, n = width_ * height_; i < n; ++i) {
            data_[i*4+0] = (fmt_ == PixelFormat::BGRA8) ? b : r;
            data_[i*4+1] = g;
            data_[i*4+2] = (fmt_ == PixelFormat::BGRA8) ? r : b;
            data_[i*4+3] = a;
        }
    } else {
        std::fill(data_.begin(), data_.end(), uint8_t(0));
    }
}

void ImageBuffer::ApplyColorSpace(ColorSpace dst) {
    if (fmt_ != PixelFormat::RGBA8 && fmt_ != PixelFormat::BGRA8) return;
    ConvertColorSpace(data_, width_, height_, cs_, dst);
    cs_ = dst;
}

void ImageBuffer::ApplyLUT(const std::vector<uint8_t>& lr,
                            const std::vector<uint8_t>& lg,
                            const std::vector<uint8_t>& lb) {
    image::ApplyLUT(data_, width_, height_, lr, lg, lb);
}

ImageBuffer ImageBuffer::Clone() const {
    ImageBuffer c;
    c.width_      = width_;
    c.height_     = height_;
    c.fmt_        = fmt_;
    c.cs_         = cs_;
    c.data_       = data_;
    c.float_data_ = float_data_;
    return c;
}

void ImageBuffer::FlipVertical() {
    int bpp = BytesPerPixel(fmt_);
    if (bpp <= 0) return;
    int stride = width_ * bpp;
    for (int y = 0; y < height_ / 2; ++y) {
        uint8_t* top = data_.data() + y * stride;
        uint8_t* bot = data_.data() + (height_ - 1 - y) * stride;
        std::swap_ranges(top, top + stride, bot);
    }
}

}  // namespace image
