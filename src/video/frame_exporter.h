#pragma once
#include "frame.h"
#include "frame_cache.h"
#include <memory>
#include <string>
#include <vector>

namespace video {

/// Writes frames to cache, disk, or video file using FFmpeg bridge.
class FrameExporter {
public:
    explicit FrameExporter(FrameCache* cache = nullptr);

    bool ToCache(const std::string& key, std::shared_ptr<Frame> frame);

    /// Save single frame to file (PNG by default via FFmpeg).
    bool ToFile(const std::string& path, const Frame& frame);

    /// Export a sequence of frames to a video file.
    bool ExportFrames(const std::string& path,
                      const std::vector<std::shared_ptr<Frame>>& frames,
                      const std::string& codec = "libx264",
                      int fps = 30);

    void SetCache(FrameCache* cache) { cache_ = cache; }

private:
    FrameCache* cache_{nullptr};
};

}  // namespace video