#pragma once
#include <cstdint>
#include <cstddef>

namespace video {

/// Lightweight non-owning view into raw frame pixel data.
struct FrameData {
    uint8_t* planes[4]{nullptr, nullptr, nullptr, nullptr};
    int      strides[4]{0, 0, 0, 0};
    int      width{0};
    int      height{0};
    int64_t  pts{0};

    bool IsValid() const { return planes[0] != nullptr && width > 0 && height > 0; }
};

}  // namespace video