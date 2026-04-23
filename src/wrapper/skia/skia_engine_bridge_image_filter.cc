#include "wrapper/skia/skia_engine_bridge_image_filter.h"
#include <cstdint>

namespace engine::bridge::skia {
    int ImageFilterCreateDropShadow(float dx, float dy, float sigma_x, float sigma_y, uint32_t color) { return -1; }
    int ImageFilterCreateBlur(float sigma_x, float sigma_y) { return -1; }
    void ImageFilterRelease(int filter_id) {}
}
