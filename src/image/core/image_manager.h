#pragma once
#include "image_buffer.h"
#include "image_descriptor.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace image {

/// Central registry: load, cache and manage ImageBuffer instances.
class ImageManager {
public:
    static ImageManager& Instance();

    /// Load image from file (uses DecoderFactory, caches by path).
    std::shared_ptr<ImageBuffer> Load(const std::string& path);

    /// Load with explicit descriptor output.
    std::shared_ptr<ImageBuffer> Load(const std::string& path,
                                      ImageDescriptor& out_desc);

    /// Force reload (bypass cache).
    std::shared_ptr<ImageBuffer> Reload(const std::string& path);

    /// Evict one entry from cache.
    void Evict(const std::string& path);

    /// Clear entire cache.
    void ClearCache();

    std::size_t CacheSize() const { return cache_.size(); }

private:
    ImageManager() = default;
    std::unordered_map<std::string,
                       std::shared_ptr<ImageBuffer>> cache_;
};

}  // namespace image
