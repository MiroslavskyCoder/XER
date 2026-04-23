#include "wrapper/skia/skia_engine_bridge_shader.h"
#include <vector>

namespace engine::bridge::skia {
    int ShaderCreateImage(const std::vector<uint32_t>& pixels, int width, int height,
                          bool repeat_x, bool repeat_y) { return -1; }
    int ShaderCreatePerlinNoise(float base_freq_x, float base_freq_y, int num_octaves, float seed) { return -1; }
    void ShaderRelease(int shader_id) {}
}
