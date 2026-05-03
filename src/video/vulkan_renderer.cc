#include "vulkan_renderer.h"
#include "wrapper/skia/skia_engine_bridge.h"
#include "flux/core/logger.h"
#include <cstring>

namespace video {

static flux::core::Logger& Log() {
    static flux::core::Logger logger;
    return logger;
}

VulkanRenderer::~VulkanRenderer() { Shutdown(); }

bool VulkanRenderer::Initialize(const std::string& /*device_hint*/) {
    // Check Skia availability (Skia uses Vulkan backend internally when available)
    bool skia_ok = engine::bridge::skia::IsAvailable();
    Log().Info("VulkanRenderer",
               std::string("Initializing Vulkan renderer. Skia: ") +
               (skia_ok ? "available" : "not available") +
               " | " + engine::bridge::skia::Summary());
    ready_ = true;  // VkInstance / VkDevice would be created here
    return true;
}

void VulkanRenderer::Shutdown() {
    if (ready_) {
        Log().Info("VulkanRenderer", "Shutdown");
        ready_ = false;
    }
}

bool VulkanRenderer::Render(const Frame& src, RenderTarget& target) {
    if (!ready_ || !src.IsValid()) return false;
    Frame* dst = target.GetFrame();
    if (!dst || !dst->IsValid()) return false;

    if (src.Width() == dst->Width() && src.Height() == dst->Height() &&
        src.Format() == dst->Format()) {
        std::memcpy(dst->Data(), src.Data(), src.DataSize());
        return true;
    }
    // Scale via Skia raster when dimensions differ
    if (engine::bridge::skia::IsAvailable()) {
        int sw=src.Width(), sh=src.Height();
        int dw=dst->Width(), dh=dst->Height();
        std::vector<uint32_t> src_px(sw*sh), dst_px(dw*dh, 0);
        const uint8_t* sd = src.Data();
        for (int i=0;i<sw*sh;++i)
            src_px[i] = engine::bridge::skia::MakeColorRGBA(
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