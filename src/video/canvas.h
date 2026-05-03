#pragma once
#include "frame.h"
#include "render_target.h"
#include "mask_system.h"
#include "fragment_cutter.h"
#include <memory>

namespace video {

/// Drawing surface: hosts a RenderTarget and applies masks / fragment cuts.
class Canvas {
public:
    Canvas() = default;
    explicit Canvas(int width, int height, PixelFormat fmt = PixelFormat::BGRA8);

    void Resize(int width, int height, PixelFormat fmt = PixelFormat::BGRA8);
    void Clear(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255);

    /// Blit a source frame at position (dx, dy).
    void Blit(const Frame& src, int dx = 0, int dy = 0);

    /// Apply a named mask from an attached MaskSystem.
    void ApplyMask(const std::string& name, MaskAction action = MaskAction::CUT_OUTSIDE);

    /// Cut fragment from current surface.
    std::unique_ptr<Frame> CutFragment(const MaskPointsVector& mask, bool mask_pixels = true);

    RenderTarget&       Target()       { return target_; }
    const RenderTarget& Target() const { return target_; }

    void AttachMaskSystem(MaskSystem* ms) { mask_system_ = ms; }

    int Width()  const { return target_.Width();  }
    int Height() const { return target_.Height(); }

private:
    RenderTarget  target_;
    MaskSystem*   mask_system_{nullptr};
    FragmentCutter cutter_;
};

}  // namespace video