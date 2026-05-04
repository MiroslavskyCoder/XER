#pragma once
#include "../core/image_buffer.h"
#include <string>

namespace image {

/// Saves ImageBuffer to disk in the requested format.
class ImageSaver {
public:
    /// quality: 0-100 (used for lossy formats: JPEG, WebP)
    bool Save(const ImageBuffer& img, const std::string& path,
              int quality = 90);

    /// Export as PNG (lossless, always RGBA8).
    bool SavePng(const ImageBuffer& img, const std::string& path);

    /// Export as JPEG.
    bool SaveJpeg(const ImageBuffer& img, const std::string& path,
                  int quality = 90);

    /// Export raw pixel data (no header).
    bool SaveRaw(const ImageBuffer& img, const std::string& path);
};

}  // namespace image
