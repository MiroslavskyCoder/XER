#include "wrapper/skia/skia_engine_bridge_codec_extra.h"

namespace engine::bridge::skia {
    bool EncodeImageWEBP(const std::vector<uint32_t>& pixels, int width, int height,
                         int quality, std::vector<uint8_t>* out_bytes) { return false; }
    bool EncodeImageGIF(const std::vector<uint32_t>& pixels, int width, int height,
                        std::vector<uint8_t>* out_bytes) { return false; }
}
