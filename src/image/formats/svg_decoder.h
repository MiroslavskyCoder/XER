#pragma once
#include "../decoding/vector_decoder.h"

namespace image {
class SvgDecoder : public VectorDecoder {
public:
    std::string FormatName() const override { return "svg"; }
    std::vector<std::string> Extensions() const override { return {"svg","svgz"}; }
    bool Probe(const uint8_t* h, std::size_t l) const override {
        return l>=1 && h[0]=='<';
    }
    std::shared_ptr<ImageBuffer> Rasterize(const std::string& path,
                                            int target_w, int target_h,
                                            ImageDescriptor& desc) override;
    std::shared_ptr<ImageBuffer> DecodeMemory(const uint8_t*, std::size_t,
                                               ImageDescriptor&) override { return nullptr; }
};
}  // namespace image
