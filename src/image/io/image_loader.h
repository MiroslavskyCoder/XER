#pragma once
#include "../core/image_buffer.h"
#include "../core/image_descriptor.h"
#include <memory>
#include <string>
#include <vector>

namespace image {

/// Loads images from disk using DecoderFactory (magic byte detection).
class ImageLoader {
public:
    ImageLoader() = default;

    /// Load a single image.
    std::shared_ptr<ImageBuffer> Load(const std::string& path,
                                       ImageDescriptor& desc);
    std::shared_ptr<ImageBuffer> Load(const std::string& path);

    /// Load all frames from a multi-page file (GIF, TIFF).
    std::vector<std::shared_ptr<ImageBuffer>>
        LoadAllPages(const std::string& path, ImageDescriptor& desc);

    /// Load from raw memory.
    std::shared_ptr<ImageBuffer> LoadMemory(const uint8_t* data, std::size_t len,
                                             ImageDescriptor& desc);
};

}  // namespace image
