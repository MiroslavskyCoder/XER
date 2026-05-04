#pragma once
#include "../decoding/raster_decoder.h"

namespace image {
class RawCr2Decoder : public RasterDecoder {
public:
    std::string FormatName() const override { return "raw_cr2"; }
    std::vector<std::string> Extensions() const override {
        return {"cr2","cr3","nef","dng","arw","raf"};
    }
    bool Probe(const uint8_t* h, std::size_t l) const override {
        // TIFF-wrapped RAW: 49 49 2A 00
        return l>=4 && h[0]==0x49&&h[1]==0x49&&h[2]==0x2A&&h[3]==0x00;
    }
    std::shared_ptr<ImageBuffer> Decode(const std::string& path,
                                         ImageDescriptor& desc) override;
    std::shared_ptr<ImageBuffer> DecodeMemory(const uint8_t*, std::size_t,
                                               ImageDescriptor&) override { return nullptr; }
};
}  // namespace image
