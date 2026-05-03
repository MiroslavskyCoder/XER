#include "darknet_layer_builder.h"

#include <cstdlib>
#include <sstream>

namespace Engine::ModelsBuilder::Reader::Darknet {

int DarknetLayerBuilder::GetInt(const CfgSection& sec,
                                 const std::string& key, int def) {
  auto it = sec.params.find(key);
  if (it == sec.params.end()) return def;
  return std::stoi(it->second);
}

float DarknetLayerBuilder::GetFloat(const CfgSection& sec,
                                     const std::string& key, float def) {
  auto it = sec.params.find(key);
  if (it == sec.params.end()) return def;
  return std::stof(it->second);
}

std::string DarknetLayerBuilder::GetStr(const CfgSection& sec,
                                         const std::string& key,
                                         const std::string& def) {
  auto it = sec.params.find(key);
  if (it == sec.params.end()) return def;
  return it->second;
}

std::vector<std::shared_ptr<Core::Layer>> DarknetLayerBuilder::BuildLayers(
    const std::vector<CfgSection>& sections) {
  std::vector<std::shared_ptr<Core::Layer>> layers;

  for (auto& sec : sections) {
    if (sec.type == "net") continue;  // Global config, not a layer

    std::shared_ptr<Core::Layer> layer;
    if      (sec.type == "convolutional")  layer = BuildConvolutional(sec);
    else if (sec.type == "connected")      layer = BuildConnected(sec);
    else if (sec.type == "maxpool")        layer = BuildMaxpool(sec);
    else if (sec.type == "avgpool")        layer = BuildAvgpool(sec);
    else if (sec.type == "route")          layer = BuildRoute(sec);
    else if (sec.type == "shortcut")       layer = BuildShortcut(sec);
    else if (sec.type == "upsample")       layer = BuildUpsample(sec);
    else if (sec.type == "yolo")           layer = BuildYolo(sec);
    else                                   layer = std::make_shared<Core::Layer>(sec.type);

    if (layer) layers.push_back(std::move(layer));
  }
  return layers;
}

std::shared_ptr<Core::Layer> DarknetLayerBuilder::BuildConvolutional(
    const CfgSection& sec) {
  auto layer = std::make_shared<Core::Layer>("Conv");
  (void)GetInt(sec, "filters", 32);
  (void)GetInt(sec, "size", 3);
  (void)GetInt(sec, "stride", 1);
  (void)GetInt(sec, "pad", 0);
  (void)GetStr(sec, "activation", "leaky");
  (void)GetInt(sec, "batch_normalize", 0);
  return layer;
}

std::shared_ptr<Core::Layer> DarknetLayerBuilder::BuildConnected(
    const CfgSection& sec) {
  auto layer = std::make_shared<Core::Layer>("Dense");
  (void)GetInt(sec, "output", 1024);
  (void)GetStr(sec, "activation", "linear");
  return layer;
}

std::shared_ptr<Core::Layer> DarknetLayerBuilder::BuildMaxpool(
    const CfgSection& sec) {
  auto layer = std::make_shared<Core::Layer>("MaxPool");
  (void)GetInt(sec, "size", 2);
  (void)GetInt(sec, "stride", 2);
  return layer;
}

std::shared_ptr<Core::Layer> DarknetLayerBuilder::BuildAvgpool(
    const CfgSection& /*sec*/) {
  return std::make_shared<Core::Layer>("GlobalAveragePool");
}

std::shared_ptr<Core::Layer> DarknetLayerBuilder::BuildRoute(
    const CfgSection& sec) {
  auto layer = std::make_shared<Core::Layer>("Route");
  (void)GetStr(sec, "layers", "");
  return layer;
}

std::shared_ptr<Core::Layer> DarknetLayerBuilder::BuildShortcut(
    const CfgSection& sec) {
  auto layer = std::make_shared<Core::Layer>("Shortcut");
  (void)GetInt(sec, "from", -3);
  return layer;
}

std::shared_ptr<Core::Layer> DarknetLayerBuilder::BuildUpsample(
    const CfgSection& sec) {
  auto layer = std::make_shared<Core::Layer>("Upsample");
  (void)GetInt(sec, "stride", 2);
  return layer;
}

std::shared_ptr<Core::Layer> DarknetLayerBuilder::BuildYolo(
    const CfgSection& sec) {
  auto layer = std::make_shared<Core::Layer>("YOLO");
  (void)GetStr(sec, "anchors", "");
  (void)GetInt(sec, "classes", 80);
  return layer;
}

}  // namespace Engine::ModelsBuilder::Reader::Darknet
