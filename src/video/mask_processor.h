#pragma once
#include "frame.h"
#include "mask_points_vector.h"
#include "video_constants.h"
#include <cstdint>

namespace video {

/// Applies a mask polygon to a frame (cut inside/outside, fill outside).
class MaskProcessor {
public:
    /// Apply mask action to frame in-place.
    /// @param frame  Target BGRA8/RGBA8 frame.
    /// @param mask   Polygon defining the masked region.
    /// @param action What to do (CUT_INSIDE / CUT_OUTSIDE / FILL_OUTSIDE).
    /// @param fill_r/g/b/a  Fill colour used for FILL_OUTSIDE.
    void Apply(Frame& frame, const MaskPointsVector& mask,
               MaskAction action,
               uint8_t fill_r = 0, uint8_t fill_g = 0,
               uint8_t fill_b = 0, uint8_t fill_a = 0);
};

}  // namespace video