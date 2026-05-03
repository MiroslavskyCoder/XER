#include "pixel_format.h"

namespace image {

int BytesPerPixel(PixelFormat fmt) {
    switch (fmt) {
        case PixelFormat::RGBA8:   return 4;
        case PixelFormat::BGRA8:   return 4;
        case PixelFormat::RGB8:    return 3;
        case PixelFormat::BGR8:    return 3;
        case PixelFormat::Gray8:   return 1;
        case PixelFormat::Gray16:  return 2;
        case PixelFormat::RGBA16:  return 8;
        case PixelFormat::RGBA32F: return 16;
        default: return 0;
    }
}

std::string PixelFormatName(PixelFormat fmt) {
    switch (fmt) {
        case PixelFormat::RGBA8:   return "rgba8";
        case PixelFormat::BGRA8:   return "bgra8";
        case PixelFormat::RGB8:    return "rgb8";
        case PixelFormat::BGR8:    return "bgr8";
        case PixelFormat::Gray8:   return "gray8";
        case PixelFormat::Gray16:  return "gray16";
        case PixelFormat::RGBA16:  return "rgba16";
        case PixelFormat::RGBA32F: return "rgba32f";
        case PixelFormat::YUV420P: return "yuv420p";
        default: return "unknown";
    }
}

PixelFormat PixelFormatFromName(const std::string& name) {
    if (name == "rgba8")   return PixelFormat::RGBA8;
    if (name == "bgra8")   return PixelFormat::BGRA8;
    if (name == "rgb8")    return PixelFormat::RGB8;
    if (name == "bgr8")    return PixelFormat::BGR8;
    if (name == "gray8")   return PixelFormat::Gray8;
    if (name == "gray16")  return PixelFormat::Gray16;
    if (name == "rgba16")  return PixelFormat::RGBA16;
    if (name == "rgba32f") return PixelFormat::RGBA32F;
    if (name == "yuv420p") return PixelFormat::YUV420P;
    return PixelFormat::Unknown;
}

}  // namespace image
