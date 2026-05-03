#pragma once
#include "loader_base.h"
#include <memory>
#include <future>
#include <string>

namespace Engine::ML::Loaders {

/// Async prefetch wrapper: loads dataset in a background thread
class LoaderPrefetcher {
public:
    explicit LoaderPrefetcher(std::unique_ptr<LoaderBase> inner)
        : inner_(std::move(inner)) {}

    /// Begin async load
    void Prefetch(const std::string& path);
    /// Block until loaded and return result
    Dataset Collect();

private:
    std::unique_ptr<LoaderBase> inner_;
    std::future<Dataset> future_;
};

}  // namespace Engine::ML::Loaders
