#pragma once
#include "rendering_engine.h"

namespace video {

/// ANGLE (cross-platform OpenGL ES on Direct3D/Vulkan/Metal) rendering backend.
class AngleRenderer : public RenderingEngine {
public:
    AngleRenderer() = default;
    ~AngleRenderer() override;

    bool Initialize(const std::string& device_hint = "") override;
    void Shutdown() override;
    bool IsReady() const override { return ready_; }
    RendererType Type() const override { return RendererType::Angle; }
    bool Render(const Frame& src, RenderTarget& target) override;

private:
    bool ready_{false};
};

}  // namespace video