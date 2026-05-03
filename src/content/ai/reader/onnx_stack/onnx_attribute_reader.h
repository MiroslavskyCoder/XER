#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace Engine::ModelsBuilder::Reader::Onnx {

class OnnxAttributeReader {
 public:
  static std::map<std::string, std::string> ReadCommonAttributes(const uint8_t* buffer,
																  size_t size);
  static uint32_t ReadUnitsHint(const std::map<std::string, std::string>& attrs,
								uint32_t fallback);
};

}  // namespace Engine::ModelsBuilder::Reader::Onnx

