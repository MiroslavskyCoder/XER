#pragma once
#include "frame.h"
#include "frame_cache.h"
#include <memory>
#include <string>

namespace video {

/// Writes frames to cache or disk.
class FrameExporter {
public:
    explicit FrameExporter(FrameCache* cache = nullptr);

    bool ToCache(const std::string& key, std::shared_ptr<Frame> frame);
    bool ToFile(const std::string& path, const Frame& frame);

    void SetCache(FrameCache* cache) { cache_ = cache; }

private:
    FrameCache* cache_{nullptr};
};

}  // namespace video