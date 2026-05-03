#pragma once
#include "raster_decoder.h"

namespace image {

/// Generic RAW sensor-data decoder (libraw stub; uses FFmpeg fallback).
class RawDecoder : public RasterDecoder {
public:
    std::string FormatName() const override { return "raw"; }
    std::vector<std::string> Extensions() const override {
        return {"dng", "nef", "arw", "raf", "orf", "rw2"};
    }
    bool Probe(const uint8_t* h, std::size_t len) const override;
    std::shared_ptr<ImageBuffer> Decode(const std::string& path,
                                         ImageDescriptor& desc) override;
    std::shared_ptr<ImageBuffer> DecodeMemory(const uint8_t* data, std::size_t len,
                                               ImageDescriptor& desc) override;
};

}  // namespace image
