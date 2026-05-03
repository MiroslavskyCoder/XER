#include "loader_binary.h"
#include <fstream>
#include <stdexcept>
#include <cstdint>

namespace Engine::ML::Loaders {

Dataset LoaderBinary::Load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open())
        throw std::runtime_error("BinaryLoader: cannot open " + path);

    uint32_t n_samples = 0, n_features = 0;
    f.read(reinterpret_cast<char*>(&n_samples),  sizeof(uint32_t));
    f.read(reinterpret_cast<char*>(&n_features), sizeof(uint32_t));

    Dataset ds;
    ds.X.resize(n_samples, std::vector<float>(n_features));
    for (uint32_t i = 0; i < n_samples; ++i)
        f.read(reinterpret_cast<char*>(ds.X[i].data()),
               n_features * sizeof(float));
    return ds;
}

}  // namespace Engine::ML::Loaders
