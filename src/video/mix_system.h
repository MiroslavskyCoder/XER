#pragma once
#include "frame.h"
#include <memory>

namespace video {

enum class MixMode : uint8_t {
    Alpha    = 0,   ///< Standard alpha compositing
    Additive = 1,   ///< Additive blending
    Multiply = 2,   ///< Multiply blend
    Screen   = 3,   ///< Screen blend
};

/// Mixes two frames together according to a blending mode.
class MixSystem {
public:
    MixSystem() = default;

    /// Mix src on top of dst in-place.
    bool Mix(Frame& dst, const Frame& src, MixMode mode = MixMode::Alpha,
             float weight = 1.0f);

    /// Create a new frame that is the mix of a and b.
    std::unique_ptr<Frame> Blend(const Frame& a, const Frame& b,
                                  MixMode mode = MixMode::Alpha,
                                  float t = 0.5f);
};

}  // namespace video