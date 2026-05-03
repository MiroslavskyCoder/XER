#pragma once
#include "pixel_format.h"
#include "color_space.h"
#include <vector>
#include <cstdint>
#include <string>

namespace image {

/// In-memory image: packed RGBA8 by default, with optional HDR float data.
class ImageBuffer {
public:
    ImageBuffer() = default;
    ImageBuffer(int width, int height, PixelFormat fmt = PixelFormat::RGBA8,
                ColorSpace cs = ColorSpace::sRGB);

    bool IsValid() const { return width_ > 0 && height_ > 0 && !data_.empty(); }
    int  Width()   const { return width_; }
    int  Height()  const { return height_; }
    PixelFormat Format()     const { return fmt_; }
    ColorSpace  GetColorSpace() const { return cs_; }

    uint8_t*       Data()       { return data_.data(); }
    const uint8_t* Data() const { return data_.data(); }
    std::size_t    DataSize()   const { return data_.size(); }

    /// Reference to the HDR float plane (RGBA32F, 16 bytes per pixel).
    std::vector<float>&       FloatData()       { return float_data_; }
    const std::vector<float>& FloatData() const { return float_data_; }

    void Allocate(int w, int h, PixelFormat fmt = PixelFormat::RGBA8,
                  ColorSpace cs = ColorSpace::sRGB);
    void Clear(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255);

    /// In-place color space conversion.
    void ApplyColorSpace(ColorSpace dst);
    void ApplyLUT(const std::vector<uint8_t>& lut_r,
                  const std::vector<uint8_t>& lut_g,
                  const std::vector<uint8_t>& lut_b);

    /// Deep copy.
    ImageBuffer Clone() const;

    /// Flip vertically (common when loading from OpenGL).
    void FlipVertical();

private:
    int                 width_  = 0;
    int                 height_ = 0;
    PixelFormat         fmt_    = PixelFormat::RGBA8;
    ColorSpace          cs_     = ColorSpace::sRGB;
    std::vector<uint8_t> data_;
    std::vector<float>   float_data_;
};

}  // namespace image
