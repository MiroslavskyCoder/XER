#pragma once
#include "frame.h"
#include "mask_points_vector.h"
#include <memory>

namespace video {

/// Cuts a rectangular bounding-box region of a frame defined by a MaskPointsVector.
class FragmentCutter {
public:
    FragmentCutter() = default;

    /// Returns a new Frame containing the bounding box of the mask polygon.
    /// Pixels outside the polygon (if mask_pixels=true) are zeroed.
    std::unique_ptr<Frame> Cut(const Frame& src, const MaskPointsVector& mask,
                                bool mask_pixels = true);

    /// Compute bounding box of a mask polygon.
    struct BBox { int x{0}, y{0}, w{0}, h{0}; };
    static BBox ComputeBBox(const MaskPointsVector& mask, int frame_w, int frame_h);
};

}  // namespace video