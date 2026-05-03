#include "frame_cache.h"

namespace video {

FrameCache::FrameCache(int max_frames) : max_frames_(max_frames) {}

void FrameCache::Store(const std::string& key, std::shared_ptr<Frame> frame) {
    if (!frame) return;
    if (store_.count(key) == 0) {
        if (static_cast<int>(store_.size()) >= max_frames_) {
            // Keep-without-eviction policy: just grow (caller must Clear() manually)
        }
        insertion_order_.push_back(key);
    }
    store_[key] = std::move(frame);
}

std::shared_ptr<Frame> FrameCache::Get(const std::string& key) const {
    auto it = store_.find(key);
    return (it != store_.end()) ? it->second : nullptr;
}

bool FrameCache::Has(const std::string& key) const {
    return store_.count(key) > 0;
}

void FrameCache::Remove(const std::string& key) {
    store_.erase(key);
}

void FrameCache::Clear() {
    store_.clear();
    insertion_order_.clear();
}

}  // namespace video