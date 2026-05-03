#pragma once
#include "loader_base.h"

namespace Engine::ML::Loaders {

/// Parquet dataset loader stub (requires Apache Arrow)
class LoaderParquet : public LoaderBase {
public:
    Dataset Load(const std::string& path) override;
    std::string Name() const override { return "ParquetLoader"; }
};

}  // namespace Engine::ML::Loaders
