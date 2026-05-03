#include "caffe_prototxt_reader.h"

namespace Engine::ModelsBuilder::Reader::Framework {

LoadResult CaffeProtoxtReader::Load(const std::string& filepath) {
  // Placeholder: full Caffe prototxt parsing requires protobuf + caffe.proto
  auto model = std::make_shared<Core::Model>("caffe_model");
  model->AddLayer(std::make_shared<Core::Layer>("CaffeConv"));
  return LoadResult{model, true, ""};
}

bool CaffeProtoxtReader::CanLoad(const std::string& filepath) const {
  return filepath.size() > 9 &&
         filepath.substr(filepath.size() - 9) == ".prototxt";
}

bool CaffeProtoxtReader::LoadWeights(const std::string& caffemodel_path) {
  weights_path_ = caffemodel_path;
  return true;
}

}  // namespace Engine::ModelsBuilder::Reader::Framework
