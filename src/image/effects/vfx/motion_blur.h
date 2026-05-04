#pragma once
#include "../../core/image_buffer.h"

namespace image {

class MotionBlurVfx {
public:
    MotionBlurVfx(double angle_deg = 0.0, int distance = 15)
        : angle_(angle_deg), distance_(distance) {}
    void Apply(ImageBuffer& img) const;

private:
    double angle_;
    int    distance_;
};

}  // namespace image
