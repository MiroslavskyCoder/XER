#pragma once
#include "../decoding/raster_decoder.h"

namespace image {
class HeifDecoder : public RasterDecoder {
public:
    std::string FormatName() const override { return "heif"; }
    std::vector<std::string> Extensions() const override { return {"heic","heif","avif"}; }
    bool Probe(const uint8_t* h, std::size_t l) const override {
        return l>=12 && h[4]=='f'&&h[5]=='t'&&h[6]=='y'&&h[7]=='p';
    }
    std::shared_ptr<ImageBuffer> Decode(const std::string& path,
                                         ImageDescriptor& desc) override;
    std::shared_ptr<ImageBuffer> DecodeMemory(const uint8_t*, std::size_t,
                                               ImageDescriptor&) override { return nullptr; }
};
}  // namespace image
