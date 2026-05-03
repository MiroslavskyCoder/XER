#pragma once
#include "rendering_engine.h"

namespace video {

/// CUDA-based rendering backend.
class CudaRenderer : public RenderingEngine {
public:
    CudaRenderer() = default;
    ~CudaRenderer() override;

    bool Initialize(const std::string& device_hint = "") override;
    void Shutdown() override;
    bool IsReady() const override { return ready_; }
    RendererType Type() const override { return RendererType::CUDA; }
    bool Render(const Frame& src, RenderTarget& target) override;

private:
    bool ready_{false};
    int  device_id_{0};
};

}  // namespace video