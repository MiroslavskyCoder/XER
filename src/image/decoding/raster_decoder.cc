#include "raster_decoder.h"

namespace image {

std::shared_ptr<ImageBuffer>
RasterDecoder::Rgb24ToBuffer(const uint8_t* src, int w, int h) {
    auto buf = std::make_shared<ImageBuffer>(w, h, PixelFormat::RGBA8);
    uint8_t* dst = buf->Data();
    for (int i = 0, n = w * h; i < n; ++i) {
        dst[i*4+0] = src[i*3+0];
        dst[i*4+1] = src[i*3+1];
        dst[i*4+2] = src[i*3+2];
        dst[i*4+3] = 255;
    }
    return buf;
}

std::shared_ptr<ImageBuffer>
RasterDecoder::Bgra8ToBuffer(const uint8_t* src, int w, int h) {
    auto buf = std::make_shared<ImageBuffer>(w, h, PixelFormat::RGBA8);
    uint8_t* dst = buf->Data();
    for (int i = 0, n = w * h; i < n; ++i) {
        dst[i*4+0] = src[i*4+2];
        dst[i*4+1] = src[i*4+1];
        dst[i*4+2] = src[i*4+0];
        dst[i*4+3] = src[i*4+3];
    }
    return buf;
}

std::shared_ptr<ImageBuffer>
RasterDecoder::Rgba8ToBuffer(const uint8_t* src, int w, int h) {
    auto buf = std::make_shared<ImageBuffer>(w, h, PixelFormat::RGBA8);
    std::memcpy(buf->Data(), src,
                static_cast<size_t>(w) * h * 4);
    return buf;
}

}  // namespace image
