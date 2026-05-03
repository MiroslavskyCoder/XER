#pragma once

#include "../core_loaders/reader_base.h"

namespace Engine::ModelsBuilder::Reader::Framework {

class OnnxModelReader : public ReaderBase {
 public:
  LoadResult Load(const std::string& filepath) override;
  bool CanLoad(const std::string& filepath) const override;
  std::string GetFormatName() const override { return "ONNX"; }
  std::string GetFileExtension() const override { return ".onnx"; }
};

}  // namespace Engine::ModelsBuilder::Reader::Framework
