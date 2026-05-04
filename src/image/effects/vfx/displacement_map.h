#pragma once
#include "../../core/image_buffer.h"

namespace image {

/// Displaces pixels using an XY displacement map stored in another ImageBuffer.
class DisplacementMap {
public:
    /// @param map   RGBA8 buffer where R channel = X displacement, G = Y.
    /// @param scale Pixel displacement at full channel value (255).
    explicit DisplacementMap(const ImageBuffer& map, float scale = 20.0f)
        : map_(map), scale_(scale) {}

    void Apply(ImageBuffer& img) const;

private:
    ImageBuffer map_;
    float       scale_;
};

}  // namespace image
