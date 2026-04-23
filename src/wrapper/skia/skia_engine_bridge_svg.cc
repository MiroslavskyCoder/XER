#include "wrapper/skia/skia_engine_bridge_svg.h"

namespace engine::bridge::skia {
    bool RasterDrawSVG(std::vector<uint32_t>* pixels, int width, int height,
                       const std::string& svg_content, float x, float y, float scale) {
        return false; // TODO: Implement SVG via SkSVGDOM
    }
}
