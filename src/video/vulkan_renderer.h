#pragma once
#include "rendering_engine.h"

namespace video {

/// Vulkan-based rendering backend.
class VulkanRenderer : public RenderingEngine {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() override;

    bool Initialize(const std::string& device_hint = "") override;
    void Shutdown() override;
    bool IsReady() const override { return ready_; }
    RendererType Type() const override { return RendererType::Vulkan; }
    bool Render(const Frame& src, RenderTarget& target) override;

private:
    bool ready_{false};
};

}  // namespace video