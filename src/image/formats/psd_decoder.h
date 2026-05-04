#pragma once
#include "../decoding/raster_decoder.h"

namespace image {
class PsdDecoder : public RasterDecoder {
public:
    std::string FormatName() const override { return "psd"; }
    std::vector<std::string> Extensions() const override { return {"psd","psb"}; }
    bool Probe(const uint8_t* h, std::size_t l) const override {
        return l>=4 && h[0]=='8'&&h[1]=='B'&&h[2]=='P'&&h[3]=='S';
    }
    std::shared_ptr<ImageBuffer> Decode(const std::string& path,
                                         ImageDescriptor& desc) override;
    std::shared_ptr<ImageBuffer> DecodeMemory(const uint8_t*, std::size_t,
                                               ImageDescriptor&) override { return nullptr; }
};
}  // namespace image
