#pragma once
#include "../../core/image_buffer.h"
#include <vector>
#include <cstdint>

namespace image {

struct FlareElement {
    float    pos;      ///< 0=source, 1=opposite
    float    size;     ///< radius in pixels
    uint32_t color;    ///< Skia-packed RGBA
    float    opacity;
};

class LensFlare {
public:
    LensFlare(int src_x, int src_y, float intensity = 1.0f);
    void Apply(ImageBuffer& img) const;

private:
    int   src_x_, src_y_;
    float intensity_;
    std::vector<FlareElement> elements_;
};

}  // namespace image
