#pragma once
#include "frame.h"
#include "frame_cache.h"
#include <memory>
#include <string>

namespace video {

/// Reads frames from cache or disk into memory.
class FrameImporter {
public:
    explicit FrameImporter(FrameCache* cache = nullptr);

    /// Load from cache by key.
    std::shared_ptr<Frame> FromCache(const std::string& key);

    /// Load from image/video file (delegates to image loaders).
    std::shared_ptr<Frame> FromFile(const std::string& path);

    void SetCache(FrameCache* cache) { cache_ = cache; }

private:
    FrameCache* cache_{nullptr};
};

}  // namespace video