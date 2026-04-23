#pragma once

#include <string>
#include <vector>

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QImage>
#endif

namespace qt6::image {

struct ImageInfo {
    int         width   = 0;
    int         height  = 0;
    int         depth   = 0;   // bits per pixel
    std::string format;        // "PNG", "JPEG", etc.
    bool        valid   = false;
};

// Load image from file, return metadata
ImageInfo GetInfo(const std::string& path);

// Resize image and save to output path (format inferred from extension)
bool Resize(const std::string& src, const std::string& dst,
            int width, int height, bool keep_aspect = true);

// Convert format: load src, save as dst (extension determines format)
bool Convert(const std::string& src, const std::string& dst, int quality = -1);

// Crop image
bool Crop(const std::string& src, const std::string& dst,
          int x, int y, int width, int height);

// Flip horizontal/vertical
bool FlipH(const std::string& src, const std::string& dst);
bool FlipV(const std::string& src, const std::string& dst);

// Rotate by degrees (90, 180, 270)
bool Rotate(const std::string& src, const std::string& dst, int degrees);

// Grayscale
bool ToGrayscale(const std::string& src, const std::string& dst);

#if ENGINE_HAS_QT6
QImage LoadQImage(const std::string& path);
bool   SaveQImage(const QImage& img, const std::string& path, int quality = -1);
#endif

}  // namespace qt6::image
