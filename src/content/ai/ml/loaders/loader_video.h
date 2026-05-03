#pragma once
#include "loader_base.h"

namespace Engine::ML::Loaders {

/// Video frame loader using OpenCV VideoCapture.
/// Each decoded frame becomes one row (HxWxC float32 pixels, [0,1]).
/// Returns empty Dataset when OpenCV is not available at compile time.
class LoaderVideo : public LoaderBase {
public:
    Dataset Load(const std::string& path) override;
    std::string Name() const override { return "VideoLoader"; }
};

}  // namespace Engine::ML::Loaders
