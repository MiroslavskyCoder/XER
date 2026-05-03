#include "pt_model_parser.h"

#include "../../models_builder/model_core/dense_layer.h"

namespace Engine::ModelsBuilder::Reader::Framework {

LoadResult PtModelParser::Load(const std::string& filepath) {
  std::vector<uint8_t> buf;
  if (!ReadFile(filepath, buf))
    return {nullptr, false, "Cannot read: " + filepath};

  if (!HasZipMagic(buf))
    return {nullptr, false, "Not a valid PyTorch archive: " + filepath};

  // Placeholder: full parsing needs libtorch or custom ZIP+pickling
  auto model = std::make_shared<Core::Model>("pytorch_model");
  model->AddLayer(std::make_shared<Core::DenseLayer>(256U));
  model->Build({1U, 256U});
  model->Compile();
  return {model, true, ""};
}

bool PtModelParser::CanLoad(const std::string& filepath) const {
  auto n = filepath.size();
  return (n >= 3 && filepath.substr(n - 3) == ".pt") ||
         (n >= 4 && filepath.substr(n - 4) == ".pth");
}

bool PtModelParser::HasZipMagic(const std::vector<uint8_t>& data) {
  if (data.size() < 4) return false;
  return data[0] == 'P' && data[1] == 'K' &&
         (data[2] == 0x03 || data[2] == 0x05 || data[2] == 0x07);
}

}  // namespace Engine::ModelsBuilder::Reader::Framework
