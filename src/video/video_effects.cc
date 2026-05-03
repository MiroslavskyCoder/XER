#include "video_effects.h"
#include <algorithm>
#include <vector>
#include <cstring>

namespace video {

static uint8_t clamp_u8(float v) {
    if (v < 0) return 0;
    if (v > 255) return 255;
    return static_cast<uint8_t>(v);
}

void VideoEffects::BrightnessContrast(Frame& frame, int brightness, float contrast) {
    if (!frame.IsValid()) return;
    int n = frame.Width() * frame.Height() * 4;
    uint8_t* px = frame.Data();
    for (int i = 0; i < n; i += 4) {
        px[i]   = clamp_u8((px[i]   - 128) * contrast + 128 + brightness);
        px[i+1] = clamp_u8((px[i+1] - 128) * contrast + 128 + brightness);
        px[i+2] = clamp_u8((px[i+2] - 128) * contrast + 128 + brightness);
    }
}

void VideoEffects::Grayscale(Frame& frame) {
    if (!frame.IsValid()) return;
    int n = frame.Width() * frame.Height();
    uint8_t* px = frame.Data();
    for (int i = 0; i < n; ++i) {
        int idx = i * 4;
        uint8_t luma = clamp_u8(0.299f*px[idx] + 0.587f*px[idx+1] + 0.114f*px[idx+2]);
        px[idx] = px[idx+1] = px[idx+2] = luma;
    }
}

void VideoEffects::BoxBlur(Frame& frame, int radius) {
    if (!frame.IsValid() || radius <= 0) return;
    int w = frame.Width(), h = frame.Height();
    std::vector<uint8_t> tmp(w * h * 4);
    const uint8_t* src = frame.Data();
    uint8_t*       dst = tmp.data();
    // Horizontal pass
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int cnt = 0;
            int r = 0, g = 0, b = 0, a = 0;
            for (int k = -radius; k <= radius; ++k) {
                int cx = std::clamp(x+k, 0, w-1);
                int idx = (y * w + cx) * 4;
                r += src[idx]; g += src[idx+1]; b += src[idx+2]; a += src[idx+3];
                ++cnt;
            }
            int idx = (y * w + x) * 4;
            dst[idx]   = static_cast<uint8_t>(r/cnt);
            dst[idx+1] = static_cast<uint8_t>(g/cnt);
            dst[idx+2] = static_cast<uint8_t>(b/cnt);
            dst[idx+3] = static_cast<uint8_t>(a/cnt);
        }
    }
    std::memcpy(frame.Data(), dst, w * h * 4);
}

void VideoEffects::FlipH(Frame& frame) {
    if (!frame.IsValid()) return;
    int w = frame.Width(), h = frame.Height();
    uint8_t* px = frame.Data();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w / 2; ++x) {
            int a = (y * w + x)     * 4;
            int b = (y * w + w-1-x) * 4;
            for (int c = 0; c < 4; ++c) std::swap(px[a+c], px[b+c]);
        }
    }
}

void VideoEffects::FlipV(Frame& frame) {
    if (!frame.IsValid()) return;
    int w = frame.Width(), h = frame.Height();
    uint8_t* px = frame.Data();
    std::vector<uint8_t> row(w * 4);
    for (int y = 0; y < h / 2; ++y) {
        int a = y * w * 4, b = (h-1-y) * w * 4;
        std::memcpy(row.data(), px + a, w * 4);
        std::memcpy(px + a,     px + b, w * 4);
        std::memcpy(px + b,     row.data(), w * 4);
    }
}

void VideoEffects::GrayscaleWithMask(Frame& frame, const MaskPointsVector& mask) {
    if (!frame.IsValid() || mask.Empty()) return;
    int w = frame.Width(), h = frame.Height();
    uint8_t* px = frame.Data();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (!mask.Contains(static_cast<float>(x), static_cast<float>(y))) continue;
            int idx = (y * w + x) * 4;
            uint8_t luma = clamp_u8(0.299f*px[idx] + 0.587f*px[idx+1] + 0.114f*px[idx+2]);
            px[idx] = px[idx+1] = px[idx+2] = luma;
        }
    }
}

}  // namespace video