#include "filter_chain.h"
#include <algorithm>

namespace image {

void FilterChain::Add(const std::string& name, FilterFn fn) {
    steps_.push_back({name, std::move(fn)});
}

void FilterChain::Remove(const std::string& name) {
    steps_.erase(std::remove_if(steps_.begin(), steps_.end(),
                 [&](const Step& s){ return s.name == name; }),
                 steps_.end());
}

void FilterChain::Clear() { steps_.clear(); }

void FilterChain::Apply(ImageBuffer& img) const {
    for (auto& s : steps_) s.fn(img);
}

}  // namespace image
