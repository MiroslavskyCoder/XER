#pragma once
#include "loader_base.h"

namespace Engine::ML::Loaders {

/// Protobuf dataset loader stub (requires libprotobuf)
class LoaderProtobuf : public LoaderBase {
public:
    Dataset Load(const std::string& path) override;
    std::string Name() const override { return "ProtobufLoader"; }
};

}  // namespace Engine::ML::Loaders
