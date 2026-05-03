#pragma once
#include "frame.h"
#include <string>

namespace video::utils {

/// Convert PixelFormat enum to human-readable string.
std::string PixelFormatName(PixelFormat fmt);

/// True if the format has an alpha channel.
bool HasAlpha(PixelFormat fmt);

/// Bytes per pixel for packed formats (BGRA8/RGBA8), 0 for planar.
int BytesPerPixel(PixelFormat fmt);

/// Clamp integer to [lo, hi].
template<typename T>
T Clamp(T v, T lo, T hi) { return v < lo ? lo : v > hi ? hi : v; }

}  // namespace video::utils