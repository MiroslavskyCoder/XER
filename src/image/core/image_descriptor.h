#pragma once
#include "pixel_format.h"
#include "color_space.h"
#include <string>
#include <unordered_map>

namespace image {

/// Metadata associated with a decoded image.
struct ImageDescriptor {
    int         width       = 0;
    int         height      = 0;
    int         depth       = 1;    ///< bits per channel
    int         channels    = 4;
    PixelFormat format      = PixelFormat::RGBA8;
    ColorSpace  color_space = ColorSpace::sRGB;
    std::string format_name;        ///< e.g. "jpeg", "png", "exr"
    std::string source_path;
    int         page_count  = 1;    ///< multi-page (GIF, TIFF)
    bool        has_alpha   = true;
    bool        is_hdr      = false;
    float       dpi_x       = 72.0f;
    float       dpi_y       = 72.0f;
    std::unordered_map<std::string, std::string> metadata; ///< EXIF/IPTC tags
};

}  // namespace image
