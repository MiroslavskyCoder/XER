#include "cuda_renderer.h"
#include <cstring>

namespace video {

CudaRenderer::~CudaRenderer() { Shutdown(); }

bool CudaRenderer::Initialize(const std::string& /*device_hint*/) {
    // TODO: cudaSetDevice, create CUDA stream
    ready_ = true;
    return true;
}

void CudaRenderer::Shutdown() { ready_ = false; }

bool CudaRenderer::Render(const Frame& src, RenderTarget& target) {
    if (!ready_ || !src.IsValid()) return false;
    Frame* dst = target.GetFrame();
    if (!dst || !dst->IsValid()) return false;
    if (src.Width() == dst->Width() && src.Height() == dst->Height() &&
        src.Format() == dst->Format()) {
        std::memcpy(dst->Data(), src.Data(), src.Width() * src.Height() * 4);
    }
    return true;
}

}  // namespace video