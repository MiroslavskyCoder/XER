#include "mask_points_vector.h"

namespace video {

bool MaskPointsVector::Contains(float px, float py) const {
    bool inside = false;
    int n = static_cast<int>(points_.size());
    for (int i = 0, j = n - 1; i < n; j = i++) {
        float xi = points_[i].x, yi = points_[i].y;
        float xj = points_[j].x, yj = points_[j].y;
        bool intersect = ((yi > py) != (yj > py)) &&
                         (px < (xj - xi) * (py - yi) / (yj - yi) + xi);
        if (intersect) inside = !inside;
    }
    return inside;
}

}  // namespace video