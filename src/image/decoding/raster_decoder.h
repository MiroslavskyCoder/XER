#pragma once
#include "base_decoder.h"

namespace image {

/// Base for all raster (pixel-data) decoders.
/// Subclasses implement format-specific logic;
/// this class provides shared RGBA8 conversion helpers.
class RasterDecoder : public BaseDecoder {
public:
    ~RasterDecoder() override = default;

protected:
    /// Convert raw RGB24 byte array to RGBA8 ImageBuffer.
    static std::shared_ptr<ImageBuffer> Rgb24ToBuffer(const uint8_t* src,
                                                       int w, int h);

    /// Convert raw BGRA8 byte array to RGBA8 ImageBuffer.
    static std::shared_ptr<ImageBuffer> Bgra8ToBuffer(const uint8_t* src,
                                                       int w, int h);

    /// Wrap an already-RGBA8 byte array.
    static std::shared_ptr<ImageBuffer> Rgba8ToBuffer(const uint8_t* src,
                                                       int w, int h);
};

}  // namespace image
