#include "frame_exporter.h"

namespace video {

FrameExporter::FrameExporter(FrameCache* cache) : cache_(cache) {}

bool FrameExporter::ToCache(const std::string& key, std::shared_ptr<Frame> frame) {
    if (!cache_ || !frame) return false;
    cache_->Store(key, std::move(frame));
    return true;
}

bool FrameExporter::ToFile(const std::string& /*path*/, const Frame& /*frame*/) {
    // TODO: delegate to image/video encoder pipeline
    return false;
}

}  // namespace video