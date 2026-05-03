#include "angle_renderer.h"
#include "wrapper/angle/angle_engine_bridge.h"
#include "wrapper/skia/skia_engine_bridge.h"
#include "flux/core/logger.h"
#include <cstring>

namespace video {

static flux::core::Logger& Log() {
    static flux::core::Logger logger;
    return logger;
}

AngleRenderer::~AngleRenderer() { Shutdown(); }

bool AngleRenderer::Initialize(const std::string& /*device_hint*/) {
    bool angle_ok = engine::bridge::angle::IsAvailable();
    Log().Info("AngleRenderer",
               std::string("ANGLE: ") +
               (angle_ok ? "available" : "not available") +
               " | " + engine::bridge::angle::Summary());
    ready_ = angle_ok;
    return ready_;
}

void AngleRenderer::Shutdown() {
    if (ready_) {
        Log().Info("AngleRenderer", "Shutdown");
        ready_ = false;
    }
}

bool AngleRenderer::Render(const Frame& src, RenderTarget& target) {
    if (!ready_ || !src.IsValid()) return false;
    Frame* dst = target.GetFrame();
    if (!dst || !dst->IsValid()) return false;

    if (src.Width() == dst->Width() && src.Height() == dst->Height() &&
        src.Format() == dst->Format()) {
        std::memcpy(dst->Data(), src.Data(), src.DataSize());
        return true;
    }
    // Fallback scale via Skia
    if (engine::bridge::skia::IsAvailable()) {
        int sw=src.Width(), sh=src.Height();
        int dw=dst->Width(), dh=dst->Height();
        std::vector<uint32_t> src_px(sw*sh), dst_px(dw*dh,0);
        const uint8_t* sd = src.Data();
        for (int i=0;i<sw*sh;++i)
            src_px[i]=engine::bridge::skia::MakeColorRGBA(
                          sd[i*4+2],sd[i*4+1],sd[i*4+0],sd[i*4+3]);
        engine::bridge::skia::RasterDrawImage(&dst_px,dw,dh,src_px,sw,sh,0,0,"src");
        uint8_t* dd = dst->Data();
        for (int i=0;i<dw*dh;++i) {
            uint32_t c=dst_px[i];
            dd[i*4+2]=(c>>24)&0xFF; dd[i*4+1]=(c>>16)&0xFF;
            dd[i*4+0]=(c>>8)&0xFF;  dd[i*4+3]=c&0xFF;
        }
        return true;
    }
    return false;
}

}  // namespace video