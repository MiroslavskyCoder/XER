#pragma once
#include "frame.h"
#include "mask_points_vector.h"

namespace video {

/// Collection of in-place video effects and filters.
class VideoEffects {
public:
    VideoEffects() = default;

    /// Adjust brightness [-255, 255] and contrast [0.0, 4.0].
    static void BrightnessContrast(Frame& frame, int brightness, float contrast);

    /// Convert to grayscale (keeps 4 channels, sets R=G=B=luma).
    static void Grayscale(Frame& frame);

    /// Simple box blur, radius in pixels.
    static void BoxBlur(Frame& frame, int radius);

    /// Flip frame horizontally or vertically.
    static void FlipH(Frame& frame);
    static void FlipV(Frame& frame);

    /// Apply effect only inside mask polygon.
    static void GrayscaleWithMask(Frame& frame, const MaskPointsVector& mask);
};

}  // namespace video