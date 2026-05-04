#pragma once
#include "../core/image_buffer.h"
#include <string>
#include <memory>

namespace image {

/// Interface for image caching backends.
class CacheInterface {
public:
    virtual ~CacheInterface() = default;
    virtual void   Store(const std::string& key,
                         std::shared_ptr<ImageBuffer> buf) = 0;
    virtual std::shared_ptr<ImageBuffer> Get(const std::string& key) = 0;
    virtual bool   Has(const std::string& key) const = 0;
    virtual void   Evict(const std::string& key) = 0;
    virtual void   Clear() = 0;
    virtual std::size_t Size() const = 0;
};

}  // namespace image
