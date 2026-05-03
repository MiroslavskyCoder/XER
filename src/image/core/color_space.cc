#include "color_space.h"
#include <cmath>
#include <stdexcept>

namespace image {

std::string ColorSpaceName(ColorSpace cs) {
    switch (cs) {
        case ColorSpace::sRGB:      return "srgb";
        case ColorSpace::LinearRGB: return "linear";
        case ColorSpace::AdobeRGB:  return "adobergb";
        case ColorSpace::ACES_CG:   return "aces_cg";
        case ColorSpace::Rec709:    return "rec709";
        case ColorSpace::Rec2020:   return "rec2020";
        case ColorSpace::P3_D65:    return "p3_d65";
        case ColorSpace::Gray:      return "gray";
        default: return "unknown";
    }
}

ColorSpace ColorSpaceFromName(const std::string& name) {
    if (name == "srgb")     return ColorSpace::sRGB;
    if (name == "linear")   return ColorSpace::LinearRGB;
    if (name == "adobergb") return ColorSpace::AdobeRGB;
    if (name == "aces_cg")  return ColorSpace::ACES_CG;
    if (name == "rec709")   return ColorSpace::Rec709;
    if (name == "rec2020")  return ColorSpace::Rec2020;
    if (name == "p3_d65")   return ColorSpace::P3_D65;
    if (name == "gray")     return ColorSpace::Gray;
    return ColorSpace::Unknown;
}

// sRGB linearisation: remove gamma
static float SRGBtoLinear(uint8_t v) {
    float s = v / 255.0f;
    return (s <= 0.04045f) ? (s / 12.92f)
                           : std::pow((s + 0.055f) / 1.055f, 2.4f);
}

// Linear to sRGB re-encode
static uint8_t LinearToSRGB(float l) {
    float s = (l <= 0.0031308f) ? (l * 12.92f)
                                : (1.055f * std::pow(l, 1.0f / 2.4f) - 0.055f);
    int v = static_cast<int>(s * 255.0f + 0.5f);
    return static_cast<uint8_t>(v < 0 ? 0 : v > 255 ? 255 : v);
}

void ConvertColorSpace(std::vector<uint8_t>& pixels, int w, int h,
                       ColorSpace src, ColorSpace dst) {
    if (src == dst) return;
    // sRGB → Linear
    if (src == ColorSpace::sRGB && dst == ColorSpace::LinearRGB) {
        for (int i = 0, n = w * h; i < n; ++i) {
            pixels[i*4+0] = static_cast<uint8_t>(SRGBtoLinear(pixels[i*4+0]) * 255.0f);
            pixels[i*4+1] = static_cast<uint8_t>(SRGBtoLinear(pixels[i*4+1]) * 255.0f);
            pixels[i*4+2] = static_cast<uint8_t>(SRGBtoLinear(pixels[i*4+2]) * 255.0f);
        }
        return;
    }
    // Linear → sRGB
    if (src == ColorSpace::LinearRGB && dst == ColorSpace::sRGB) {
        for (int i = 0, n = w * h; i < n; ++i) {
            pixels[i*4+0] = LinearToSRGB(pixels[i*4+0] / 255.0f);
            pixels[i*4+1] = LinearToSRGB(pixels[i*4+1] / 255.0f);
            pixels[i*4+2] = LinearToSRGB(pixels[i*4+2] / 255.0f);
        }
        return;
    }
    // Rec709 and sRGB use same gamma (approx)
    if ((src == ColorSpace::sRGB && dst == ColorSpace::Rec709) ||
        (src == ColorSpace::Rec709 && dst == ColorSpace::sRGB)) {
        return;  // no-op
    }
    // Other conversions: pass-through (full OCIO not linked)
}

void ApplyLUT(std::vector<uint8_t>& pixels, int w, int h,
              const std::vector<uint8_t>& lut_r,
              const std::vector<uint8_t>& lut_g,
              const std::vector<uint8_t>& lut_b) {
    if (lut_r.size() < 256 || lut_g.size() < 256 || lut_b.size() < 256) return;
    for (int i = 0, n = w * h; i < n; ++i) {
        pixels[i*4+0] = lut_r[pixels[i*4+0]];
        pixels[i*4+1] = lut_g[pixels[i*4+1]];
        pixels[i*4+2] = lut_b[pixels[i*4+2]];
    }
}

}  // namespace image
