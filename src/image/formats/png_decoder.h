#pragma once
#include "../decoding/raster_decoder.h"

namespace image {
class PngDecoder : public RasterDecoder {
public:
    std::string FormatName() const override { return "png"; }
    std::vector<std::string> Extensions() const override { return {"png"}; }
    bool Probe(const uint8_t* h, std::size_t l) const override {
        return l>=4 && h[0]==0x89 && h[1]=='P' && h[2]=='N' && h[3]=='G';
    }
    std::shared_ptr<ImageBuffer> Decode(const std::string& path,
                                         ImageDescriptor& desc) override;
    std::shared_ptr<ImageBuffer> DecodeMemory(const uint8_t* data, std::size_t len,
                                               ImageDescriptor& desc) override;
};
}  // namespace image
