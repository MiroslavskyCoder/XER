#pragma once
#include "frame.h"
#include <unordered_map>
#include <string>
#include <memory>
#include <deque>

namespace video {

/// Stores frames by key without eviction (explicit release only).
class FrameCache {
public:
    explicit FrameCache(int max_frames = 512);

    void Store(const std::string& key, std::shared_ptr<Frame> frame);
    std::shared_ptr<Frame> Get(const std::string& key) const;
    bool Has(const std::string& key) const;
    void Remove(const std::string& key);
    void Clear();
    int  Size() const { return static_cast<int>(store_.size()); }

private:
    int max_frames_;
    std::unordered_map<std::string, std::shared_ptr<Frame>> store_;
    std::deque<std::string> insertion_order_;
};

}  // namespace video