#pragma once
#include "../core/image_buffer.h"
#include "../core/image_descriptor.h"
#include <memory>
#include <string>
#include <vector>

namespace image {

/// Abstract base for all image decoders.
class BaseDecoder {
public:
    virtual ~BaseDecoder() = default;

    /// Unique format identifier, e.g. "jpeg", "png", "exr".
    virtual std::string FormatName() const = 0;

    /// File extensions this decoder handles (lower-case, no dot).
    virtual std::vector<std::string> Extensions() const = 0;

    /// Magic bytes probe: return true if this decoder can handle the data.
    virtual bool Probe(const uint8_t* header, std::size_t len) const = 0;

    /// Decode from file path.
    virtual std::shared_ptr<ImageBuffer> Decode(const std::string& path,
                                                 ImageDescriptor& desc) = 0;

    /// Decode from memory buffer.
    virtual std::shared_ptr<ImageBuffer> DecodeMemory(const uint8_t* data,
                                                       std::size_t len,
                                                       ImageDescriptor& desc) = 0;
};

}  // namespace image
