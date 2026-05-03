#pragma once
#include <vector>
#include <string>

namespace Engine::ML::Loaders {

/// Result of a dataset load: rows × features
struct Dataset {
    std::vector<std::vector<float>> X;  ///< Feature matrix
    std::vector<int>                y;  ///< Labels (may be empty)
};

/// Abstract base for all data loaders
class LoaderBase {
public:
    virtual ~LoaderBase() = default;

    /// Load dataset from file/path
    virtual Dataset Load(const std::string& path) = 0;

    /// Human-readable name
    virtual std::string Name() const = 0;
};

}  // namespace Engine::ML::Loaders
