#include "video_utils.h"

namespace video::utils {

std::string PixelFormatName(PixelFormat fmt) {
    switch (fmt) {
        case PixelFormat::BGRA8:   return "BGRA8";
        case PixelFormat::RGBA8:   return "RGBA8";
        case PixelFormat::NV12:    return "NV12";
        case PixelFormat::YUV420P: return "YUV420P";
    }
    return "Unknown";
}

bool HasAlpha(PixelFormat fmt) {
    return fmt == PixelFormat::BGRA8 || fmt == PixelFormat::RGBA8;
}

int BytesPerPixel(PixelFormat fmt) {
    switch (fmt) {
        case PixelFormat::BGRA8:
        case PixelFormat::RGBA8: return 4;
        default: return 0;  // planar
    }
}

}  // namespace video::utils