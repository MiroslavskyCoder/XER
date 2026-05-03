#include "raw_decoder.h"
#include "wrapper/ffmpeg/ffmpeg_engine_bridge.h"
#include "flux/core/logger.h"
#include <cstring>

namespace image {

bool RawDecoder::Probe(const uint8_t* h, std::size_t len) const {
    // DNG/NEF/ARW all wrap in TIFF container: 49 49 2A 00
    return len >= 4 && h[0]==0x49 && h[1]==0x49 && h[2]==0x2A && h[3]==0x00;
}

std::shared_ptr<ImageBuffer> RawDecoder::Decode(const std::string& path,
                                                  ImageDescriptor& desc) {
    if (!engine::bridge::ffmpeg::IsAvailable()) return nullptr;
    engine::bridge::ffmpeg::MediaInfo info;
    std::string err;
    if (!engine::bridge::ffmpeg::ProbeMedia(path, &info, &err)) {
        flux::core::Logger().Error("RawDecoder", "Probe failed: " + err);
        return nullptr;
    }
    std::vector<engine::bridge::ffmpeg::VideoFrameInfo> frames;
    if (!engine::bridge::ffmpeg::DecodeVideoFrames(path, 0, 1, &frames, &err) ||
        frames.empty()) {
        flux::core::Logger().Error("RawDecoder", "Decode failed: " + err);
        return nullptr;
    }
    auto& fr = frames[0];
    desc.width  = fr.width;
    desc.height = fr.height;
    desc.format_name = "raw";
    desc.format = PixelFormat::RGBA8;

    if (fr.pixel_format == "rgba" || fr.pixel_format == "bgra") {
        auto* d = fr.data.data();
        if (fr.pixel_format == "bgra") return Bgra8ToBuffer(d, fr.width, fr.height);
        return Rgba8ToBuffer(d, fr.width, fr.height);
    }
    return Rgb24ToBuffer(fr.data.data(), fr.width, fr.height);
}

std::shared_ptr<ImageBuffer> RawDecoder::DecodeMemory(const uint8_t*, std::size_t,
                                                        ImageDescriptor&) {
    // LibRaw memory decode would go here; not bridged yet
    return nullptr;
}

}  // namespace image
