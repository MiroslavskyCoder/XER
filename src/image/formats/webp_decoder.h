#pragma once
#include "../decoding/raster_decoder.h"

namespace image {
class WebpDecoder : public RasterDecoder {
public:
    std::string FormatName() const override { return "webp"; }
    std::vector<std::string> Extensions() const override { return {"webp"}; }
    bool Probe(const uint8_t* h, std::size_t l) const override {
        return l>=12 && h[0]=='R'&&h[1]=='I'&&h[2]=='F'&&h[3]=='F'&&
               h[8]=='W'&&h[9]=='E'&&h[10]=='B'&&h[11]=='P';
    }
    std::shared_ptr<ImageBuffer> Decode(const std::string& path,
                                         ImageDescriptor& desc) override;
    std::shared_ptr<ImageBuffer> DecodeMemory(const uint8_t*, std::size_t,
                                               ImageDescriptor&) override { return nullptr; }
};
}  // namespace image
