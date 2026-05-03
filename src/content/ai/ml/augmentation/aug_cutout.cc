#include "aug_cutout.h"
#include <algorithm>
#include <random>

namespace Engine::ML::Augmentation {

AugCutout::AugCutout(int pw, int ph, float fill) : patch_w_(pw), patch_h_(ph), fill_(fill) {}

bool AugCutout::Apply(ImageSample& sample) {
    if (!enabled_ || sample.pixels.empty()) return false;
    const int W = sample.width, H = sample.height, C = sample.channels;
    thread_local std::mt19937 rng{std::random_device{}()};
    int cx = std::uniform_int_distribution<int>(0, W - 1)(rng);
    int cy = std::uniform_int_distribution<int>(0, H - 1)(rng);
    int x0 = std::max(0, cx - patch_w_ / 2);
    int y0 = std::max(0, cy - patch_h_ / 2);
    int x1 = std::min(W, x0 + patch_w_);
    int y1 = std::min(H, y0 + patch_h_);
    for (int c = 0; c < C; ++c)
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                sample.pixels[c * H * W + y * W + x] = fill_;
    return true;
}

}  // namespace Engine::ML::Augmentation
