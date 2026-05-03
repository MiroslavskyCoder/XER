#include "mask_processor.h"

namespace video {

void MaskProcessor::Apply(Frame& frame, const MaskPointsVector& mask,
                           MaskAction action,
                           uint8_t fill_r, uint8_t fill_g,
                           uint8_t fill_b, uint8_t fill_a) {
    if (!frame.IsValid() || mask.Empty()) return;
    if (frame.Format() != PixelFormat::BGRA8 &&
        frame.Format() != PixelFormat::RGBA8) return;

    int w = frame.Width(), h = frame.Height();
    uint8_t* px = frame.Data();

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            bool inside = mask.Contains(static_cast<float>(x), static_cast<float>(y));
            bool should_fill = (action == MaskAction::CUT_INSIDE  &&  inside) ||
                               (action == MaskAction::CUT_OUTSIDE  && !inside) ||
                               (action == MaskAction::FILL_OUTSIDE && !inside);
            if (should_fill) {
                int idx = (y * w + x) * 4;
                px[idx+0] = fill_r;
                px[idx+1] = fill_g;
                px[idx+2] = fill_b;
                px[idx+3] = fill_a;
            }
        }
    }
}

}  // namespace video