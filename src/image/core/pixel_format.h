#pragma once
#include <cstdint>
#include <string>

namespace image {

enum class PixelFormat : uint8_t {
    Unknown = 0,
    RGBA8,      ///< 8-bit per channel, R-G-B-A
    BGRA8,      ///< 8-bit per channel, B-G-R-A (native Skia)
    RGB8,       ///< 8-bit per channel, R-G-B
    BGR8,       ///< 8-bit per channel, B-G-R
    Gray8,      ///< 8-bit greyscale
    Gray16,     ///< 16-bit greyscale
    RGBA16,     ///< 16-bit per channel
    RGBA32F,    ///< 32-bit float per channel (HDR/EXR)
    YUV420P,    ///< planar YUV 4:2:0
};

/// Bytes per pixel for packed formats (returns 0 for planar).
int BytesPerPixel(PixelFormat fmt);
std::string PixelFormatName(PixelFormat fmt);
PixelFormat PixelFormatFromName(const std::string& name);

}  // namespace image
