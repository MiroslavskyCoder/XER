#include "cuda_renderer.h"
#include "wrapper/cuda/cuda_engine_bridge.h"
#include "wrapper/skia/skia_engine_bridge.h"
#include "flux/core/logger.h"
#include <cstring>

namespace video {

static flux::core::Logger& Log() {
    static flux::core::Logger logger;
    return logger;
}

CudaRenderer::~CudaRenderer() { Shutdown(); }

bool CudaRenderer::Initialize(const std::string& device_hint) {
    if (!engine::bridge::cuda::IsAvailable()) {
        Log().Warning("CudaRenderer", "CUDA not available: " +
                      engine::bridge::cuda::Summary());
        return false;
    }
    device_hint_ = device_hint;
    ready_ = true;
    Log().Info("CudaRenderer", "Initialized. " + engine::bridge::cuda::Summary());
    return true;
}

void CudaRenderer::Shutdown() {
    if (ready_) {
        Log().Info("CudaRenderer", "Shutdown");
        ready_ = false;
    }
}

bool CudaRenderer::Render(const Frame& src, RenderTarget& target) {
    if (!ready_ || !src.IsValid()) return false;
    Frame* dst = target.GetFrame();
    if (!dst || !dst->IsValid()) return false;

    // For matching dimensions: high-throughput memcpy (GPU would use cudaMemcpy)
    if (src.Width() == dst->Width() && src.Height() == dst->Height() &&
        src.Format() == dst->Format()) {
        std::memcpy(dst->Data(), src.Data(), src.DataSize());
        return true;
    }

    // Dimension mismatch: resize via Skia RasterDrawImage (CPU bridge)
    if (engine::bridge::skia::IsAvailable()) {
        int sw = src.Width(), sh = src.Height();
        int dw = dst->Width(), dh = dst->Height();
        std::vector<uint32_t> src_px(sw * sh), dst_px(dw * dh, 0);
        const uint8_t* sd = src.Data();
        for (int i = 0; i < sw * sh; ++i)
            src_px[i] = engine::bridge::skia::MakeColorRGBA(
                            sd[i*4+2], sd[i*4+1], sd[i*4+0], sd[i*4+3]);
        engine::bridge::skia::RasterDrawImage(&dst_px, dw, dh, src_px, sw, sh, 0, 0, "src");
        uint8_t* dd = dst->Data();
        for (int i = 0; i < dw * dh; ++i) {
            uint32_t c = dst_px[i];
            dd[i*4+2]=(c>>24)&0xFF; dd[i*4+1]=(c>>16)&0xFF;
            dd[i*4+0]=(c>> 8)&0xFF; dd[i*4+3]=c&0xFF;
        }
        return true;
    }
    return false;
}

}  // namespace video