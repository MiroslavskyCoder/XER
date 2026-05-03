#pragma once
#include "rendering_engine.h"
#include <string>

namespace video {

/// CUDA-based rendering backend (uses engine::bridge::cuda).
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
    bool        ready_{false};
    std::string device_hint_;
};

}  // namespace video