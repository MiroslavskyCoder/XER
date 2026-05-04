#pragma once
#include "../../core/image_buffer.h"
#include <array>

namespace image {

class ColorMatrixFilter {
public:
    using Matrix = std::array<float, 20>;  // 4x5 RGBA+offset

    explicit ColorMatrixFilter(const Matrix& m) : matrix_(m) {}

    static ColorMatrixFilter Grayscale();
    static ColorMatrixFilter Sepia(float amount = 1.0f);
    static ColorMatrixFilter Invert();
    static ColorMatrixFilter Saturate(float sat);

    void Apply(ImageBuffer& img) const;

private:
    Matrix matrix_;
};

}  // namespace image
