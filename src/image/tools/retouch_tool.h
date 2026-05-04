#pragma once
#include "../core/image_buffer.h"
#include <vector>

namespace image {

class RetouchTool {
public:
    /// Clone-stamp: copy region (sx,sy,size) to (dx,dy).
    static void CloneStamp(ImageBuffer& img,
                            int sx, int sy, int dx, int dy, int radius);

    /// Simple inpaint: fill masked area by diffusion from edges.
    /// mask: greyscale same size as img; white(255) = area to fill.
    static void Inpaint(ImageBuffer& img, const ImageBuffer& mask,
                        int iterations = 50);

    /// Healing brush: blend source region into destination.
    static void HealingBrush(ImageBuffer& img,
                              int src_x, int src_y,
                              int dst_x, int dst_y, int radius);

    /// Spot removal: fill circle with average surrounding color.
    static void SpotRemove(ImageBuffer& img, int cx, int cy, int radius);
};

}  // namespace image
