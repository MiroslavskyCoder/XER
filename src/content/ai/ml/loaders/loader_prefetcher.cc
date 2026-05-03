#include "loader_prefetcher.h"

namespace Engine::ML::Loaders {

void LoaderPrefetcher::Prefetch(const std::string& path) {
    LoaderBase* p = inner_.get();
    future_ = std::async(std::launch::async, [p, path]() {
        return p->Load(path);
    });
}

Dataset LoaderPrefetcher::Collect() {
    return future_.get();
}

}  // namespace Engine::ML::Loaders
