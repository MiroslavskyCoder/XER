#pragma once
#include "../core/image_buffer.h"
#include <memory>
#include <vector>
#include <string>

namespace image {

struct Layer {
    std::string  name;
    ImageBuffer  buffer;
    float        opacity  = 1.0f;
    bool         visible  = true;
    std::string  blend_mode = "src_over";  ///< Skia blend mode name
};

/// Non-destructive layer stack (Photoshop-style).
class LayersManager {
public:
    LayersManager() = default;

    /// Add a layer on top.
    void AddLayer(const std::string& name, ImageBuffer buf,
                  float opacity = 1.0f,
                  const std::string& blend_mode = "src_over");

    void RemoveLayer(const std::string& name);
    void SetVisible(const std::string& name, bool v);
    void SetOpacity(const std::string& name, float opacity);
    void SetBlendMode(const std::string& name, const std::string& mode);

    /// Flatten all visible layers into one RGBA8 buffer.
    ImageBuffer Flatten(int width, int height) const;

    std::size_t Count() const { return layers_.size(); }
    const std::vector<Layer>& Layers() const { return layers_; }

private:
    std::vector<Layer> layers_;
};

}  // namespace image
