#pragma once
#include "../core/image_buffer.h"
#include <array>
#include <vector>

namespace image {

struct Histogram {
    std::array<uint32_t, 256> r{};
    std::array<uint32_t, 256> g{};
    std::array<uint32_t, 256> b{};
    std::array<uint32_t, 256> a{};
    std::array<uint32_t, 256> luminance{};
};

class HistogramAnalyzer {
public:
    /// Compute histogram from RGBA8 buffer.
    Histogram Compute(const ImageBuffer& img) const;

    /// Mean luminance [0-255].
    float MeanLuminance(const Histogram& h) const;

    /// Histogram equalization (returns new buffer).
    ImageBuffer Equalize(const ImageBuffer& img) const;
};

}  // namespace image
