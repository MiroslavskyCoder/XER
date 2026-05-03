#include "vino_ir_converter.h"

namespace Engine::ModelsBuilder::Reader::Vino {

std::shared_ptr<Core::Model> VinoIrConverter::Convert(
    const std::vector<VinoLayerInfo>& layers,
    const VinoBinReader& bin_reader) {
  auto model = std::make_shared<Core::Model>("openvino_model");

  for (auto& info : layers) {
    auto layer = ConvertLayer(info, bin_reader);
    if (layer) model->AddLayer(layer);
  }

  return model;
}

std::shared_ptr<Core::Layer> VinoIrConverter::ConvertLayer(
    const VinoLayerInfo& info, const VinoBinReader& /*bin*/) {
  // Map OpenVINO layer type → XER layer name
  static const std::unordered_map<std::string, std::string> kTypeMap = {
    {"Convolution",            "Conv"},
    {"Deconvolution",          "ConvTranspose"},
    {"FullyConnected",         "Dense"},
    {"ReLU",                   "Relu"},
    {"Sigmoid",                "Sigmoid"},
    {"TanH",                   "Tanh"},
    {"Clamp",                  "Clamp"},
    {"SoftMax",                "Softmax"},
    {"BatchNormalization",     "BatchNormalization"},
    {"MaxPool",                "MaxPool"},
    {"AvgPool",                "AveragePool"},
    {"Concat",                 "Concat"},
    {"Eltwise",                "Add"},
    {"Reshape",                "Reshape"},
    {"Flatten",                "Flatten"},
    {"Transpose",              "Transpose"},
    {"LSTM",                   "LSTM"},
    {"GRU",                    "GRU"},
  };

  auto it = kTypeMap.find(info.type);
  std::string layer_type = (it != kTypeMap.end()) ? it->second : info.type;

  auto layer = std::make_shared<Core::Layer>(layer_type);
  return layer;
}

}  // namespace Engine::ModelsBuilder::Reader::Vino
