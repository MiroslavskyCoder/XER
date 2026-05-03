#include "keras_h5_reader.h"

namespace Engine::ModelsBuilder::Reader::Framework {

LoadResult KerasH5Reader::Load(const std::string& filepath) {
  // Placeholder: full HDF5 support requires libhdf5 + JSON model_config parsing
  auto model = std::make_shared<Core::Model>("keras_model");
  model->AddLayer(std::make_shared<Core::Layer>("Dense"));
  return LoadResult{model, true, ""};
}

bool KerasH5Reader::CanLoad(const std::string& filepath) const {
  if (filepath.size() < 3) return false;
  auto ext = filepath.substr(filepath.size() - 3);
  return ext == ".h5" || (filepath.size() > 6 &&
         filepath.substr(filepath.size() - 6) == ".keras");
}

}  // namespace Engine::ModelsBuilder::Reader::Framework
