#pragma once

#include "../model_core/layer.h"
#include <memory>
#include <string>

namespace Engine::ModelsBuilder::LayerFactory {

/// @brief Batch normalization layer
class LayerBatchNorm : public Core::Layer {
 public:
  struct Config {
    int   num_features{1};
    float eps{1e-5f};
    float momentum{0.1f};
    bool  affine{true};
  };

  explicit LayerBatchNorm(const std::string& name, Config config);
  const Config& GetConfig() const { return config_; }

 private:
  Config config_;
};

std::shared_ptr<Core::Layer> MakeBatchNorm(const std::string& name,
                                              LayerBatchNorm::Config config);

}  // namespace Engine::ModelsBuilder::LayerFactory
