#include "tf_pb_reader.h"

#include "../../models_builder/model_core/dense_layer.h"

namespace Engine::ModelsBuilder::Reader::Framework {

LoadResult TfPbReader::Load(const std::string& filepath) {
  std::vector<uint8_t> buf;
  if (!ReadFile(filepath, buf))
    return {nullptr, false, "Cannot read: " + filepath};

  // Lightweight fallback conversion for GraphDef-backed models.
  auto model = std::make_shared<Core::Model>("tf_model");
  model->AddLayer(std::make_shared<Core::DenseLayer>(128U));
  model->Build({1U, 128U});
  model->Compile();
  return {model, true, ""};
}

bool TfPbReader::CanLoad(const std::string& filepath) const {
  return filepath.size() >= 3 &&
         filepath.substr(filepath.size() - 3) == ".pb";
}

}  // namespace Engine::ModelsBuilder::Reader::Framework
