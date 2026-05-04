#pragma once
#include "../decoding/raster_decoder.h"

namespace image {
class ExrDecoder : public RasterDecoder {
public:
    std::string FormatName() const override { return "exr"; }
    std::vector<std::string> Extensions() const override { return {"exr"}; }
    bool Probe(const uint8_t* h, std::size_t l) const override {
        return l>=4 && h[0]==0x76&&h[1]==0x2F&&h[2]==0x31&&h[3]==0x01;
    }
    std::shared_ptr<ImageBuffer> Decode(const std::string& path,
                                         ImageDescriptor& desc) override;
    std::shared_ptr<ImageBuffer> DecodeMemory(const uint8_t*, std::size_t,
                                               ImageDescriptor&) override { return nullptr; }
};
}  // namespace image
