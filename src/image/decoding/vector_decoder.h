#pragma once
#include "base_decoder.h"

namespace image {

/// Base for vector-format decoders (SVG etc.) — rasterises to ImageBuffer.
class VectorDecoder : public BaseDecoder {
public:
    ~VectorDecoder() override = default;

    /// Rasterize at given target size; pass 0 to use document's natural size.
    virtual std::shared_ptr<ImageBuffer> Rasterize(const std::string& path,
                                                    int target_width,
                                                    int target_height,
                                                    ImageDescriptor& desc) = 0;

    // BaseDecoder bridge — calls Rasterize with natural size
    std::shared_ptr<ImageBuffer> Decode(const std::string& path,
                                         ImageDescriptor& desc) override {
        return Rasterize(path, 0, 0, desc);
    }
};

}  // namespace image
