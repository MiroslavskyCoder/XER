#pragma once
#include "../decoding/raster_decoder.h"

namespace image {
class JpegDecoder : public RasterDecoder {
public:
    std::string FormatName() const override { return "jpeg"; }
    std::vector<std::string> Extensions() const override { return {"jpg","jpeg","jpe"}; }
    bool Probe(const uint8_t* h, std::size_t l) const override {
        return l >= 3 && h[0]==0xFF && h[1]==0xD8 && h[2]==0xFF;
    }
    std::shared_ptr<ImageBuffer> Decode(const std::string& path,
                                         ImageDescriptor& desc) override;
    std::shared_ptr<ImageBuffer> DecodeMemory(const uint8_t* data, std::size_t len,
                                               ImageDescriptor& desc) override;
};
}  // namespace image
