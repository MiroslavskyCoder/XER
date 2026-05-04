#include "svg_decoder.h"
#include "wrapper/skia/skia_engine_bridge.h"
#include "flux/core/logger.h"
#include <fstream>
#include <sstream>

namespace image {

std::shared_ptr<ImageBuffer>
SvgDecoder::Rasterize(const std::string& path, int target_w, int target_h,
                       ImageDescriptor& desc) {
    // Use a default canvas size when none requested
    int w = target_w > 0 ? target_w : 512;
    int h = target_h > 0 ? target_h : 512;

    desc.width = w;  desc.height = h;
    desc.format = PixelFormat::RGBA8;  desc.format_name = "svg";

    // Raster: fill white background, Skia bridge doesn't expose SVG rendering
    // so we create a blank canvas (calling code can draw SVG paths on top)
    auto buf = std::make_shared<ImageBuffer>(w, h, PixelFormat::RGBA8);
    buf->Clear(255, 255, 255, 255);
    if (engine::bridge::skia::IsAvailable()) {
        // Indicate SVG presence via a neutral grey placeholder
        std::vector<uint32_t> px(w * h, engine::bridge::skia::MakeColorRGBA(200,200,200,255));
        engine::bridge::skia::RasterClear(&px, w, h,
                                           engine::bridge::skia::MakeColorRGBA(240,240,240,255));
        uint8_t* dst = buf->Data();
        for (int i=0,n=w*h;i<n;++i) {
            uint32_t c = px[i];
            dst[i*4+0]=(c>>24)&0xFF; dst[i*4+1]=(c>>16)&0xFF;
            dst[i*4+2]=(c>>8)&0xFF;  dst[i*4+3]=c&0xFF;
        }
    }
    flux::core::Logger().Info("SvgDecoder",
                              "Loaded " + path + " as raster placeholder " +
                              std::to_string(w) + "x" + std::to_string(h));
    return buf;
}

}  // namespace image
