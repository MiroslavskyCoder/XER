#include "wrapper/skia/skia_engine_bridge_lottie.h"

namespace engine::bridge::skia {
    int LottieLoadAnimation(const std::string& json_content) { return -1; }
    double LottieGetDuration(int anim_id) { return 0.0; }
    bool LottieRenderFrame(int anim_id, std::vector<uint32_t>* pixels, int width, int height,
                           double time_seconds, float x, float y, float scale) { return false; }
    void LottieRelease(int anim_id) {}
}
