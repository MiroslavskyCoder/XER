#pragma once
#include "../../core/image_buffer.h"
#include <functional>
#include <vector>
#include <string>

namespace image {

using FilterFn = std::function<void(ImageBuffer&)>;

/// Non-destructive chain of filter operations applied in order.
class FilterChain {
public:
    void Add(const std::string& name, FilterFn fn);
    void Remove(const std::string& name);
    void Clear();
    void Apply(ImageBuffer& img) const;
    std::size_t Size() const { return steps_.size(); }

private:
    struct Step { std::string name; FilterFn fn; };
    std::vector<Step> steps_;
};

}  // namespace image
