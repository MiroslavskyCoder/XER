#pragma once
#include "../core/image_buffer.h"

namespace image {

class TransformTool {
public:
    /// Bilinear resize.
    static ImageBuffer Resize(const ImageBuffer& img, int new_w, int new_h);

    /// Nearest-neighbour resize (fast).
    static ImageBuffer ResizeNN(const ImageBuffer& img, int new_w, int new_h);

    /// Scale preserving aspect ratio to fit within max_w×max_h.
    static ImageBuffer FitInto(const ImageBuffer& img, int max_w, int max_h);

    /// Affine 2D transform (3×2 matrix [a,b,c; d,e,f]).
    static ImageBuffer Affine(const ImageBuffer& img,
                               float a, float b, float c,
                               float d, float e, float f,
                               int out_w, int out_h);
};

}  // namespace image
