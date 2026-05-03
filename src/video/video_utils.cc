#include "video_utils.h"
#include "wrapper/ffmpeg/ffmpeg_engine_bridge.h"
#include "wrapper/skia/skia_engine_bridge.h"

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
        default: return 0;
    }
}

/// Returns list of codecs from FFmpeg bridge (decoded + encoded names).
std::vector<std::string> AvailableVideoCodecs() {
    std::vector<std::string> result;
    if (!engine::bridge::ffmpeg::IsAvailable()) return result;
    for (const auto& c : engine::bridge::ffmpeg::Codecs()) {
        if (c.media_type == "video") result.push_back(c.name);
    }
    return result;
}

/// Returns FFmpeg MediaInfo for the file at path.
bool ProbeFile(const std::string& path,
               engine::bridge::ffmpeg::MediaInfo* out_info,
               std::string* out_error) {
    if (!engine::bridge::ffmpeg::IsAvailable()) {
        if (out_error) *out_error = "FFmpeg not available";
        return false;
    }
    return engine::bridge::ffmpeg::ProbeMedia(path, out_info, out_error);
}

/// Blend two BGRA8 pixels using Skia BlendModeApply.
uint32_t BlendPixels(const uint8_t* dst_bgra, const uint8_t* src_bgra,
                     const std::string& skia_blend_mode) {
    if (!engine::bridge::skia::IsAvailable()) return 0;
    uint32_t d = engine::bridge::skia::MakeColorRGBA(
                     dst_bgra[2], dst_bgra[1], dst_bgra[0], dst_bgra[3]);
    uint32_t s = engine::bridge::skia::MakeColorRGBA(
                     src_bgra[2], src_bgra[1], src_bgra[0], src_bgra[3]);
    return engine::bridge::skia::BlendModeApply(skia_blend_mode, d, s);
}

}  // namespace video::utils