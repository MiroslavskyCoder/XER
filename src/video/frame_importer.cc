#include "frame_importer.h"

namespace video {

FrameImporter::FrameImporter(FrameCache* cache) : cache_(cache) {}

std::shared_ptr<Frame> FrameImporter::FromCache(const std::string& key) {
    if (!cache_) return nullptr;
    return cache_->Get(key);
}

std::shared_ptr<Frame> FrameImporter::FromFile(const std::string& /*path*/) {
    // TODO: delegate to image/video loader pipeline
    return nullptr;
}

}  // namespace video