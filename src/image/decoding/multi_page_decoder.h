#pragma once
#include "base_decoder.h"
#include <vector>

namespace image {

/// Extended interface for multi-page/animated decoders (GIF, TIFF, APNG).
class MultiPageDecoder : public BaseDecoder {
public:
    ~MultiPageDecoder() override = default;

    /// Returns the number of pages/frames in the file.
    virtual int PageCount(const std::string& path) = 0;

    /// Decode a specific page (0-based).
    virtual std::shared_ptr<ImageBuffer> DecodePage(const std::string& path,
                                                     int page,
                                                     ImageDescriptor& desc) = 0;

    /// Decode all pages.
    virtual std::vector<std::shared_ptr<ImageBuffer>>
        DecodeAll(const std::string& path, ImageDescriptor& desc);

    // BaseDecoder: decode first page
    std::shared_ptr<ImageBuffer> Decode(const std::string& path,
                                         ImageDescriptor& desc) override {
        return DecodePage(path, 0, desc);
    }
};

}  // namespace image
