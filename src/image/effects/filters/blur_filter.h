#pragma once
#include "../../core/image_buffer.h"

namespace image {

/// Gaussian and box blur via Skia bridge.
class BlurFilter {
public:
    /// sigma: standard deviation for Gaussian (pixels)
    explicit BlurFilter(double sigma = 2.0, bool gaussian = true)
        : sigma_(sigma), gaussian_(gaussian) {}

    void Apply(ImageBuffer& img) const;

private:
    double sigma_;
    bool   gaussian_;
};

}  // namespace image
