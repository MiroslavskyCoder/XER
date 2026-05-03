#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace image {

enum class ColorSpace {
    Unknown,
    sRGB,
    LinearRGB,
    AdobeRGB,
    ACES_CG,     ///< ACES CG linear
    Rec709,
    Rec2020,
    P3_D65,
    Gray,
};

std::string ColorSpaceName(ColorSpace cs);
ColorSpace ColorSpaceFromName(const std::string& name);

/// Convert packed RGBA8 buffer between color spaces (CPU gamma math).
void ConvertColorSpace(std::vector<uint8_t>& pixels, int w, int h,
                       ColorSpace src, ColorSpace dst);

/// Apply a 1D LUT (256 entries per channel, interleaved RGB) to RGBA8.
void ApplyLUT(std::vector<uint8_t>& pixels, int w, int h,
              const std::vector<uint8_t>& lut_r,
              const std::vector<uint8_t>& lut_g,
              const std::vector<uint8_t>& lut_b);

}  // namespace image
