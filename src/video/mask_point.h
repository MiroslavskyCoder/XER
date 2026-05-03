#pragma once

namespace video {

/// A single 2D point defining a mask vertex.
struct MaskPoint {
    float x{0.0f};
    float y{0.0f};

    MaskPoint() = default;
    MaskPoint(float x, float y) : x(x), y(y) {}
};

}  // namespace video