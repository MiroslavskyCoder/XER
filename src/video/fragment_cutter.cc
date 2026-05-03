#include "fragment_cutter.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace video {

FragmentCutter::BBox FragmentCutter::ComputeBBox(const MaskPointsVector& mask,
                                                   int frame_w, int frame_h) {
    if (mask.Empty()) return {};
    float min_x = mask.Points()[0].x, max_x = min_x;
    float min_y = mask.Points()[0].y, max_y = min_y;
    for (auto& p : mask.Points()) {
        min_x = std::min(min_x, p.x);
        max_x = std::max(max_x, p.x);
        min_y = std::min(min_y, p.y);
        max_y = std::max(max_y, p.y);
    }
    BBox bb;
    bb.x = std::max(0, static_cast<int>(std::floor(min_x)));
    bb.y = std::max(0, static_cast<int>(std::floor(min_y)));
    bb.w = std::min(frame_w - bb.x, static_cast<int>(std::ceil(max_x)) - bb.x);
    bb.h = std::min(frame_h - bb.y, static_cast<int>(std::ceil(max_y)) - bb.y);
    return bb;
}

std::unique_ptr<Frame> FragmentCutter::Cut(const Frame& src,
                                            const MaskPointsVector& mask,
                                            bool mask_pixels) {
    if (!src.IsValid() || mask.Empty()) return nullptr;
    if (src.Format() != PixelFormat::BGRA8 && src.Format() != PixelFormat::RGBA8)
        return nullptr;

    BBox bb = ComputeBBox(mask, src.Width(), src.Height());
    if (bb.w <= 0 || bb.h <= 0) return nullptr;

    auto out = std::make_unique<Frame>();
    out->Allocate(bb.w, bb.h, src.Format());

    const uint8_t* src_px = src.Data();
    uint8_t*       dst_px = out->Data();

    for (int y = 0; y < bb.h; ++y) {
        for (int x = 0; x < bb.w; ++x) {
            int sx = bb.x + x, sy = bb.y + y;
            int src_idx = (sy * src.Width() + sx) * 4;
            int dst_idx = (y  * bb.w       + x)  * 4;
            bool inside = !mask_pixels || mask.Contains(static_cast<float>(sx),
                                                         static_cast<float>(sy));
            if (inside) {
                std::memcpy(dst_px + dst_idx, src_px + src_idx, 4);
            } else {
                std::memset(dst_px + dst_idx, 0, 4);
            }
        }
    }
    return out;
}

}  // namespace video