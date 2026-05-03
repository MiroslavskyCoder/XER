#include "multi_page_decoder.h"

namespace image {

std::vector<std::shared_ptr<ImageBuffer>>
MultiPageDecoder::DecodeAll(const std::string& path, ImageDescriptor& desc) {
    int n = PageCount(path);
    std::vector<std::shared_ptr<ImageBuffer>> pages;
    pages.reserve(n);
    for (int i = 0; i < n; ++i)
        pages.push_back(DecodePage(path, i, desc));
    return pages;
}

}  // namespace image
