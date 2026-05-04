#pragma once
#include "../decoding/multi_page_decoder.h"

namespace image {
class GifDecoder : public MultiPageDecoder {
public:
    std::string FormatName() const override { return "gif"; }
    std::vector<std::string> Extensions() const override { return {"gif"}; }
    bool Probe(const uint8_t* h, std::size_t l) const override {
        return l>=3 && h[0]=='G' && h[1]=='I' && h[2]=='F';
    }
    int PageCount(const std::string& path) override;
    std::shared_ptr<ImageBuffer> DecodePage(const std::string& path, int page,
                                             ImageDescriptor& desc) override;
    std::shared_ptr<ImageBuffer> DecodeMemory(const uint8_t*, std::size_t,
                                               ImageDescriptor&) override { return nullptr; }
};
}  // namespace image
