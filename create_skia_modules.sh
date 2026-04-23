#!/bin/bash

DIR="src/wrapper/skia"
mkdir -p "$DIR"

# 1. PDF
cat << 'H_EOF' > "$DIR/skia_engine_bridge_pdf.h"
#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace engine::bridge::skia {
    // Begins a new PDF document. Returns an opaque ID or handle.
    int PDFBeginDocument(const std::string& author, const std::string& title);
    // Begins a new page in the given document. Returns a Canvas ID.
    int PDFBeginPage(int doc_id, float width, float height);
    // Ends the current page.
    bool PDFEndPage(int doc_id);
    // Finalizes the document and serializes it to bytes.
    bool PDFEndDocument(int doc_id, std::vector<uint8_t>* out_bytes);
}
H_EOF

cat << 'C_EOF' > "$DIR/skia_engine_bridge_pdf.cc"
#include "wrapper/skia/skia_engine_bridge_pdf.h"
#if ENGINE_HAS_SKIA_BRIDGE
#include <SkDocument.h>
#include <SkCanvas.h>
#endif

namespace engine::bridge::skia {
    int PDFBeginDocument(const std::string& author, const std::string& title) { return -1; /* TODO: Implement */ }
    int PDFBeginPage(int doc_id, float width, float height) { return -1; /* TODO: Implement */ }
    bool PDFEndPage(int doc_id) { return false; /* TODO: Implement */ }
    bool PDFEndDocument(int doc_id, std::vector<uint8_t>* out_bytes) { return false; /* TODO: Implement */ }
}
C_EOF

# 2. SVG
cat << 'H_EOF' > "$DIR/skia_engine_bridge_svg.h"
#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace engine::bridge::skia {
    // Renders an SVG string to the target pixel buffer
    bool RasterDrawSVG(std::vector<uint32_t>* pixels, int width, int height,
                       const std::string& svg_content, float x, float y, float scale);
}
H_EOF

cat << 'C_EOF' > "$DIR/skia_engine_bridge_svg.cc"
#include "wrapper/skia/skia_engine_bridge_svg.h"

namespace engine::bridge::skia {
    bool RasterDrawSVG(std::vector<uint32_t>* pixels, int width, int height,
                       const std::string& svg_content, float x, float y, float scale) {
        return false; // TODO: Implement SVG via SkSVGDOM
    }
}
C_EOF

# 3. Lottie
cat << 'H_EOF' > "$DIR/skia_engine_bridge_lottie.h"
#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace engine::bridge::skia {
    // Load a Lottie animation from JSON. Returns a handle.
    int LottieLoadAnimation(const std::string& json_content);
    // Get animation duration in seconds
    double LottieGetDuration(int anim_id);
    // Render a specific time/frame of the animation to the pixel buffer
    bool LottieRenderFrame(int anim_id, std::vector<uint32_t>* pixels, int width, int height,
                           double time_seconds, float x, float y, float scale);
    // Release the animation
    void LottieRelease(int anim_id);
}
H_EOF

cat << 'C_EOF' > "$DIR/skia_engine_bridge_lottie.cc"
#include "wrapper/skia/skia_engine_bridge_lottie.h"

namespace engine::bridge::skia {
    int LottieLoadAnimation(const std::string& json_content) { return -1; }
    double LottieGetDuration(int anim_id) { return 0.0; }
    bool LottieRenderFrame(int anim_id, std::vector<uint32_t>* pixels, int width, int height,
                           double time_seconds, float x, float y, float scale) { return false; }
    void LottieRelease(int anim_id) {}
}
C_EOF

# 4. Shader
cat << 'H_EOF' > "$DIR/skia_engine_bridge_shader.h"
#pragma once
#include <cstdint>

namespace engine::bridge::skia {
    // Create an Image Shader handle
    int ShaderCreateImage(const std::vector<uint32_t>& pixels, int width, int height,
                          bool repeat_x, bool repeat_y);
    // Create a Perlin Noise Shader handle
    int ShaderCreatePerlinNoise(float base_freq_x, float base_freq_y, int num_octaves, float seed);
    // Release shader
    void ShaderRelease(int shader_id);
}
H_EOF

cat << 'C_EOF' > "$DIR/skia_engine_bridge_shader.cc"
#include "wrapper/skia/skia_engine_bridge_shader.h"
#include <vector>

namespace engine::bridge::skia {
    int ShaderCreateImage(const std::vector<uint32_t>& pixels, int width, int height,
                          bool repeat_x, bool repeat_y) { return -1; }
    int ShaderCreatePerlinNoise(float base_freq_x, float base_freq_y, int num_octaves, float seed) { return -1; }
    void ShaderRelease(int shader_id) {}
}
C_EOF

# 5. Image Filters (Real-time)
cat << 'H_EOF' > "$DIR/skia_engine_bridge_image_filter.h"
#pragma once

namespace engine::bridge::skia {
    // Create a Drop Shadow ImageFilter handle
    int ImageFilterCreateDropShadow(float dx, float dy, float sigma_x, float sigma_y, uint32_t color);
    // Create a Blur ImageFilter handle
    int ImageFilterCreateBlur(float sigma_x, float sigma_y);
    // Release image filter
    void ImageFilterRelease(int filter_id);
}
H_EOF

cat << 'C_EOF' > "$DIR/skia_engine_bridge_image_filter.cc"
#include "wrapper/skia/skia_engine_bridge_image_filter.h"
#include <cstdint>

namespace engine::bridge::skia {
    int ImageFilterCreateDropShadow(float dx, float dy, float sigma_x, float sigma_y, uint32_t color) { return -1; }
    int ImageFilterCreateBlur(float sigma_x, float sigma_y) { return -1; }
    void ImageFilterRelease(int filter_id) {}
}
C_EOF

# 6. WebP / GIF
cat << 'H_EOF' > "$DIR/skia_engine_bridge_codec_extra.h"
#pragma once
#include <cstdint>
#include <vector>

namespace engine::bridge::skia {
    bool EncodeImageWEBP(const std::vector<uint32_t>& pixels, int width, int height,
                         int quality, std::vector<uint8_t>* out_bytes);
    bool EncodeImageGIF(const std::vector<uint32_t>& pixels, int width, int height,
                        std::vector<uint8_t>* out_bytes);
}
H_EOF

cat << 'C_EOF' > "$DIR/skia_engine_bridge_codec_extra.cc"
#include "wrapper/skia/skia_engine_bridge_codec_extra.h"

namespace engine::bridge::skia {
    bool EncodeImageWEBP(const std::vector<uint32_t>& pixels, int width, int height,
                         int quality, std::vector<uint8_t>* out_bytes) { return false; }
    bool EncodeImageGIF(const std::vector<uint32_t>& pixels, int width, int height,
                        std::vector<uint8_t>* out_bytes) { return false; }
}
C_EOF

# 7. M44 (3D Projections)
cat << 'H_EOF' > "$DIR/skia_engine_bridge_m44.h"
#pragma once
#include <array>

namespace engine::bridge::skia {
    using Matrix4x4 = std::array<float, 16>;

    Matrix4x4 M44Identity();
    Matrix4x4 M44Translate(float x, float y, float z);
    Matrix4x4 M44Scale(float x, float y, float z);
    Matrix4x4 M44RotateX(float degrees);
    Matrix4x4 M44RotateY(float degrees);
    Matrix4x4 M44RotateZ(float degrees);
    Matrix4x4 M44Multiply(const Matrix4x4& a, const Matrix4x4& b);
}
H_EOF

cat << 'C_EOF' > "$DIR/skia_engine_bridge_m44.cc"
#include "wrapper/skia/skia_engine_bridge_m44.h"

namespace engine::bridge::skia {
    Matrix4x4 M44Identity() { return {}; }
    Matrix4x4 M44Translate(float x, float y, float z) { return {}; }
    Matrix4x4 M44Scale(float x, float y, float z) { return {}; }
    Matrix4x4 M44RotateX(float degrees) { return {}; }
    Matrix4x4 M44RotateY(float degrees) { return {}; }
    Matrix4x4 M44RotateZ(float degrees) { return {}; }
    Matrix4x4 M44Multiply(const Matrix4x4& a, const Matrix4x4& b) { return {}; }
}
C_EOF

echo "Skia extension modules created."
