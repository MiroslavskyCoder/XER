#pragma once
#include "frame.h"
#include "canvas.h"
#include <vector>
#include <memory>

namespace video {

/// Blends multiple frames (layers) into a single output frame.
class CompositingEngine {
public:
    CompositingEngine() = default;

    struct Layer {
        std::shared_ptr<Frame> frame;
        int  z_order{0};
        int  x_offset{0}, y_offset{0};
        float opacity{1.0f};  ///< 0.0–1.0
    };

    void AddLayer(std::shared_ptr<Frame> frame, int z = 0,
                  int x = 0, int y = 0, float opacity = 1.0f);
    void ClearLayers();
    int  LayerCount() const { return static_cast<int>(layers_.size()); }

    /// Composite all layers onto output frame. Sorts by z_order.
    bool Composite(Frame& output);

private:
    std::vector<Layer> layers_;
};

}  // namespace video