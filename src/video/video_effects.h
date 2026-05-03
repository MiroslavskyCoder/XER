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

    /// Convert to grayscale (keeps 4 channels, sets R=G=B=luma). Uses Skia if available.
    static void Grayscale(Frame& frame);

    /// Box blur via Skia FilterBlur, CPU fallback.
    static void BoxBlur(Frame& frame, int radius);

    /// Gaussian blur (Skia FilterGaussianBlur).
    static void GaussianBlur(Frame& frame, double sigma);

    /// Unsharp-mask style sharpen (Skia FilterSharpen).
    static void Sharpen(Frame& frame, double amount = 1.5);

    /// Sobel edge detection (Skia FilterEdgeDetect).
    static void EdgeDetect(Frame& frame);

    /// Emboss relief effect (Skia FilterEmboss).
    static void Emboss(Frame& frame);

    /// Vignette darkening (Skia FilterVignette).
    static void Vignette(Frame& frame, double strength = 0.5, double radius = 0.7);

    /// Chromatic aberration / RGB shift (Skia FilterChromaticAberration).
    static void ChromaticAberration(Frame& frame, double offset = 2.0);

    /// Pixelate with block_size (Skia FilterPixelate).
    static void Pixelate(Frame& frame, int block_size = 8);

    /// Oil paint effect (Skia FilterOilPaint).
    static void OilPaint(Frame& frame, int radius = 4);

    /// Motion blur (Skia FilterMotionBlur), angle in degrees.
    static void MotionBlur(Frame& frame, double angle = 0.0, int distance = 10);

    /// Flip frame horizontally or vertically.
    static void FlipH(Frame& frame);
    static void FlipV(Frame& frame);

    /// Apply grayscale only inside mask polygon.
    static void GrayscaleWithMask(Frame& frame, const MaskPointsVector& mask);
};

}  // namespace video