#include "angle_renderer.h"
#include <cstring>

namespace video {

AngleRenderer::~AngleRenderer() { Shutdown(); }

bool AngleRenderer::Initialize(const std::string& /*device_hint*/) {
    // TODO: EGL/GLES context creation via ANGLE
    ready_ = true;
    return true;
}

void AngleRenderer::Shutdown() { ready_ = false; }

bool AngleRenderer::Render(const Frame& src, RenderTarget& target) {
    if (!ready_ || !src.IsValid()) return false;
    Frame* dst = target.GetFrame();
    if (!dst || !dst->IsValid()) return false;
    std::memcpy(dst->Data(), src.Data(), src.Width() * src.Height() * 4);
    return true;
}

}  // namespace video