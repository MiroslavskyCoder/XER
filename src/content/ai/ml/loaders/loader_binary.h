#pragma once
#include "loader_base.h"
#include <cstddef>

namespace Engine::ML::Loaders {

/// Raw binary float loader: reads flat array of float32 from file
/// File layout: [n_samples: uint32][n_features: uint32][floats...]
class LoaderBinary : public LoaderBase {
public:
    Dataset Load(const std::string& path) override;
    std::string Name() const override { return "BinaryLoader"; }
};

}  // namespace Engine::ML::Loaders
