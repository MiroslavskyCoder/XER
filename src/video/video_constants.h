#pragma once
#include <cstdint>

namespace video {

constexpr int kDefaultFps        = 30;
constexpr int kMaxFrameCacheSize = 512;    ///< Max cached frames in memory
constexpr int kMaxWidth          = 7680;   ///< 8K
constexpr int kMaxHeight         = 4320;
constexpr int kDefaultBitDepth   = 8;

enum class RendererType : uint8_t { CUDA = 0, Vulkan = 1, Angle = 2, CPU = 3 };
enum class MaskAction   : uint8_t { CUT_INSIDE = 0, CUT_OUTSIDE = 1, FILL_OUTSIDE = 2 };

}  // namespace video