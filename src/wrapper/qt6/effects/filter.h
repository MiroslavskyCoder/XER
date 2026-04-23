#pragma once

#include <string>
#include <vector>

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

namespace qt6::effects {

struct BlurOptions {
    double  radius     = 5.0;
    bool    gaussian   = true;   // true = Gaussian, false = box
};

struct DropShadowOptions {
    double       blur_radius = 5.0;
    double       offset_x    = 2.0;
    double       offset_y    = 2.0;
    std::string  color       = "#80000000";  // ARGB CSS
};

struct GlowOptions {
    double      radius = 8.0;
    std::string color  = "#ffffffff";
};

// Apply Gaussian/box blur to image file
bool ApplyBlur(const std::string& src, const std::string& dst,
               const BlurOptions& opts = {});

// Drop shadow composite
bool ApplyDropShadow(const std::string& src, const std::string& dst,
                     const DropShadowOptions& opts = {});

// Glow effect
bool ApplyGlow(const std::string& src, const std::string& dst,
               const GlowOptions& opts = {});

// Brightness/contrast (0-200, 100 = unchanged)
bool AdjustBrightnessContrast(const std::string& src, const std::string& dst,
                               int brightness = 100, int contrast = 100);

// Tint: multiply all pixels by color
bool ApplyTint(const std::string& src, const std::string& dst,
               const std::string& color);

}  // namespace qt6::effects
