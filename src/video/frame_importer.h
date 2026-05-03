#pragma once
#include "frame.h"
#include "frame_cache.h"
#include <memory>
#include <string>
#include <vector>

namespace video {

/// Reads frames from cache or disk using FFmpeg bridge.
class FrameImporter {
public:
    explicit FrameImporter(FrameCache* cache = nullptr);

    /// Load first video frame from file (any format FFmpeg supports).
    std::shared_ptr<Frame> FromFile(const std::string& path);

    /// Load all video frames from file.
    std::vector<std::shared_ptr<Frame>> AllFramesFromFile(const std::string& path);

    /// Try cache first; decode from file on miss and store result.
    std::shared_ptr<Frame> FromFileOrCache(const std::string& path);

    /// Load from cache by key.
    std::shared_ptr<Frame> FromCache(const std::string& key);

    void SetCache(FrameCache* cache) { cache_ = cache; }

private:
    FrameCache* cache_{nullptr};
};

}  // namespace video