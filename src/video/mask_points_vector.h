#pragma once
#include "mask_point.h"
#include <vector>

namespace video {

/// An ordered list of MaskPoint forming a closed polygon for masking.
class MaskPointsVector {
public:
    void AddPoint(MaskPoint p)   { points_.push_back(p); }
    void Clear()                  { points_.clear(); }
    bool Empty() const            { return points_.empty(); }
    int  Size()  const            { return static_cast<int>(points_.size()); }

    const std::vector<MaskPoint>& Points() const { return points_; }

    /// True if point (px, py) is inside the polygon (ray-casting).
    bool Contains(float px, float py) const;

private:
    std::vector<MaskPoint> points_;
};

}  // namespace video