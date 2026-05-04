#include "image_manager.h"
#include "../io/image_loader.h"
#include "flux/core/logger.h"

namespace image {

ImageManager& ImageManager::Instance() {
    static ImageManager inst;
    return inst;
}

std::shared_ptr<ImageBuffer> ImageManager::Load(const std::string& path) {
    ImageDescriptor desc;
    return Load(path, desc);
}

std::shared_ptr<ImageBuffer> ImageManager::Load(const std::string& path,
                                                  ImageDescriptor& out_desc) {
    auto it = cache_.find(path);
    if (it != cache_.end()) return it->second;
    return Reload(path);
    (void)out_desc;
}

std::shared_ptr<ImageBuffer> ImageManager::Reload(const std::string& path) {
    ImageLoader loader;
    ImageDescriptor desc;
    auto buf = loader.Load(path, desc);
    if (buf) cache_[path] = buf;
    return buf;
}

void ImageManager::Evict(const std::string& path) {
    cache_.erase(path);
}

void ImageManager::ClearCache() {
    cache_.clear();
}

}  // namespace image
