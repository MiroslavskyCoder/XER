#include "loader_batcher.h"
#include <numeric>
#include <algorithm>
#include <random>

namespace Engine::ML::Loaders {

std::vector<Batch> LoaderBatcher::MakeBatches(const Dataset& ds) {
    const size_t n = ds.X.size();
    std::vector<size_t> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    if (shuffle_) {
        std::mt19937 rng(seed_);
        std::shuffle(indices.begin(), indices.end(), rng);
    }
    std::vector<Batch> batches;
    for (size_t start = 0; start < n; start += batch_size_) {
        Batch b;
        for (size_t i = start; i < std::min(start + batch_size_, n); ++i) {
            b.X.push_back(ds.X[indices[i]]);
            if (!ds.y.empty()) b.y.push_back(ds.y[indices[i]]);
        }
        batches.push_back(std::move(b));
    }
    return batches;
}

}  // namespace Engine::ML::Loaders
