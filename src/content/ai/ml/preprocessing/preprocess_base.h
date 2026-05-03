#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace Engine::ML::Preprocessing {

/// Abstract base for all preprocessors (operates on float vectors)
class PreprocessBase {
public:
    virtual ~PreprocessBase() = default;

    /// Fit to data (compute statistics / vocabulary etc.)
    virtual void Fit(const std::vector<std::vector<float>>& X) = 0;

    /// Transform data using fitted parameters
    virtual std::vector<std::vector<float>> Transform(
        const std::vector<std::vector<float>>& X) const = 0;

    /// Fit + Transform in one call
    virtual std::vector<std::vector<float>> FitTransform(
        const std::vector<std::vector<float>>& X) {
        Fit(X);
        return Transform(X);
    }

    virtual bool IsFitted() const = 0;
    virtual std::string Name()    const = 0;
};

}  // namespace Engine::ML::Preprocessing
