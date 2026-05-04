#include "image_saver.h"
#include "wrapper/ffmpeg/ffmpeg_engine_bridge.h"
#include "flux/core/logger.h"
#include <filesystem>
#include <fstream>
#include <algorithm>

namespace image {

bool ImageSaver::Save(const ImageBuffer& img, const std::string& path, int quality) {
    std::string ext = std::filesystem::path(path).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext == ".jpg" || ext == ".jpeg") return SaveJpeg(img, path, quality);
    if (ext == ".raw")  return SaveRaw(img, path);
    return SavePng(img, path);  // default
}

bool ImageSaver::SavePng(const ImageBuffer& img, const std::string& path) {
    if (!engine::bridge::ffmpeg::IsAvailable()) return false;
    if (!img.IsValid()) return false;
    int w = img.Width(), h = img.Height();
    // Build VideoFrameInfo-compatible frame
    engine::bridge::ffmpeg::EncodeVideoParams p;
    p.output_path  = path;
    p.codec_name   = "png";
    p.width        = w;
    p.height       = h;
    p.fps_num      = 1;
    p.fps_den      = 1;
    p.pixel_format = "rgba";

    engine::bridge::ffmpeg::VideoFrameInfo fr;
    fr.width  = w; fr.height = h;
    fr.pixel_format = "rgba";
    fr.line_sizes = {w * 4};
    fr.data.assign(img.Data(), img.Data() + img.DataSize());

    std::string err;
    if (!engine::bridge::ffmpeg::EncodeVideoFrames(p, {fr}, &err)) {
        flux::core::Logger().Error("ImageSaver", "PNG save failed: " + err);
        return false;
    }
    return true;
}

bool ImageSaver::SaveJpeg(const ImageBuffer& img, const std::string& path,
                           int quality) {
    if (!engine::bridge::ffmpeg::IsAvailable()) return false;
    if (!img.IsValid()) return false;
    int w = img.Width(), h = img.Height();
    engine::bridge::ffmpeg::EncodeVideoParams p;
    p.output_path  = path;
    p.codec_name   = "mjpeg";
    p.width        = w;
    p.height       = h;
    p.fps_num      = 1;
    p.fps_den      = 1;
    p.pixel_format = "rgba";
    (void)quality;

    engine::bridge::ffmpeg::VideoFrameInfo fr;
    fr.width=w; fr.height=h;
    fr.pixel_format="rgba";
    fr.line_sizes = {w * 4};
    fr.data.assign(img.Data(), img.Data()+img.DataSize());

    std::string err;
    if (!engine::bridge::ffmpeg::EncodeVideoFrames(p, {fr}, &err)) {
        flux::core::Logger().Error("ImageSaver", "JPEG save failed: " + err);
        return false;
    }
    return true;
}

bool ImageSaver::SaveRaw(const ImageBuffer& img, const std::string& path) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(img.Data()), img.DataSize());
    return f.good();
}

}  // namespace image
