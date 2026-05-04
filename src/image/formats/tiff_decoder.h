#pragma once
#include "../decoding/multi_page_decoder.h"

namespace image {
class TiffDecoder : public MultiPageDecoder {
public:
    std::string FormatName() const override { return "tiff"; }
    std::vector<std::string> Extensions() const override { return {"tif","tiff"}; }
    bool Probe(const uint8_t* h, std::size_t l) const override {
        return l>=4 && ((h[0]==0x49&&h[1]==0x49&&h[2]==0x2A&&h[3]==0x00)||
                        (h[0]==0x4D&&h[1]==0x4D&&h[2]==0x00&&h[3]==0x2A));
    }
    int PageCount(const std::string& path) override;
    std::shared_ptr<ImageBuffer> DecodePage(const std::string& path, int page,
                                             ImageDescriptor& desc) override;
    std::shared_ptr<ImageBuffer> DecodeMemory(const uint8_t*, std::size_t,
                                               ImageDescriptor&) override { return nullptr; }
};
}  // namespace image
