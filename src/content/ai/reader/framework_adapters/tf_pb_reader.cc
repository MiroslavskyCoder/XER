#include "tf_pb_reader.h"

namespace Engine::ModelsBuilder::Reader::Framework {

LoadResult TfPbReader::Load(const std::string& filepath) {
  std::vector<uint8_t> buf;
  if (!ReadFile(filepath, buf))
    return {nullptr, false, "Cannot read: " + filepath};

  // Placeholder: real impl decodes GraphDef protobuf
  auto model = std::make_shared<Core::Model>("tf_model");
  model->AddLayer(std::make_shared<Core::Layer>("MatMul"));
  return {model, true, ""};
}

bool TfPbReader::CanLoad(const std::string& filepath) const {
  return filepath.size() >= 3 &&
         filepath.substr(filepath.size() - 3) == ".pb";
}

}  // namespace Engine::ModelsBuilder::Reader::Framework
