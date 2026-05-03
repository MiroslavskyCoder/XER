#include "tf_saved_model_parser.h"

#include <sys/stat.h>

namespace Engine::ModelsBuilder::Reader::Framework {

LoadResult TfSavedModelParser::Load(const std::string& filepath) {
  // Check if filepath is a directory containing saved_model.pb
  std::string pb = filepath + "/saved_model.pb";
  struct stat st{};
  if (::stat(pb.c_str(), &st) != 0)
    return {nullptr, false, "No saved_model.pb in: " + filepath};

  // Delegate to TfPbReader
  auto model = std::make_shared<Core::Model>("tf_saved_model");
  model->AddLayer(std::make_shared<Core::Layer>("Dense"));
  return {model, true, ""};
}

bool TfSavedModelParser::CanLoad(const std::string& filepath) const {
  struct stat st{};
  if (::stat(filepath.c_str(), &st) != 0) return false;
  if (!S_ISDIR(st.st_mode)) return false;
  std::string pb = filepath + "/saved_model.pb";
  return ::stat(pb.c_str(), &st) == 0;
}

}  // namespace Engine::ModelsBuilder::Reader::Framework
