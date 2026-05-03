#pragma once
#include "frame.h"
#include "render_target.h"
#include "video_constants.h"
#include <string>

namespace video {

/// Abstract rendering engine backend (CUDA / Vulkan / Angle / CPU).
class RenderingEngine {
public:
    virtual ~RenderingEngine() = default;

    virtual bool Initialize(const std::string& device_hint = "")         = 0;
    virtual void Shutdown()                                                = 0;
    virtual bool IsReady() const                                           = 0;

    virtual RendererType Type() const                                      = 0;

    /// Render source frame into render target.
    virtual bool Render(const Frame& src, RenderTarget& target)            = 0;

    /// Optional: blit rendered output to display / window handle.
    virtual void Present() {}
};

}  // namespace video