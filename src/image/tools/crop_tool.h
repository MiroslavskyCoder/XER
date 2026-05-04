#pragma once
#include "../core/image_buffer.h"

namespace image {

class CropTool {
public:
    /// Crop to region [x, y, width, height].
    static ImageBuffer Crop(const ImageBuffer& img, int x, int y, int w, int h);

    /// Pad image to new dimensions, centering the original.
    static ImageBuffer Pad(const ImageBuffer& img, int new_w, int new_h,
                           uint8_t r=0, uint8_t g=0, uint8_t b=0, uint8_t a=255);

    /// Rotate 90° clockwise n times.
    static ImageBuffer Rotate90(const ImageBuffer& img, int times = 1);

    /// Flip horizontal.
    static ImageBuffer FlipH(const ImageBuffer& img);

    /// Flip vertical.
    static ImageBuffer FlipV(const ImageBuffer& img);
};

}  // namespace image
