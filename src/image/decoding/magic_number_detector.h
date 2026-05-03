#pragma once
#include <cstdint>
#include <cstddef>
#include <string>

namespace image {

/// Identifies image type from the first bytes of a file/stream.
class MagicNumberDetector {
public:
    /// Returns format name ("jpeg", "png", "gif", "webp", "bmp",
    /// "tiff", "exr", "heif", "svg", "psd", "raw") or empty string.
    static std::string Detect(const uint8_t* header, std::size_t len);

    /// Convenience: open file, read header bytes, return format.
    static std::string DetectFile(const std::string& path);
};

}  // namespace image
