#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace video {

enum class PixelFormat : uint8_t { BGRA8 = 0, RGBA8 = 1, NV12 = 2, YUV420P = 3 };

struct FrameMetadata {
    int width{0};
    int height{0};
    PixelFormat format{PixelFormat::BGRA8};
    int64_t pts{0};   ///< presentation timestamp (microseconds)
    int64_t dts{0};   ///< decode timestamp
    double fps{0.0};
    bool is_keyframe{false};
};

class Frame {
public:
    Frame() = default;
    Frame(int w, int h, PixelFormat fmt);
    ~Frame() = default;

    bool Allocate(int w, int h, PixelFormat fmt);
    void Release();
    bool IsValid() const { return !data_.empty(); }

    int Width()  const { return meta_.width; }
    int Height() const { return meta_.height; }
    PixelFormat Format() const { return meta_.format; }
    int64_t Pts() const { return meta_.pts; }
    void SetPts(int64_t pts) { meta_.pts = pts; }

    uint8_t*       Data()       { return data_.data(); }
    const uint8_t* Data() const { return data_.data(); }
    size_t         DataSize() const { return data_.size(); }

    int Stride() const;   ///< bytes per row for plane 0
    const FrameMetadata& Meta() const { return meta_; }
    FrameMetadata& Meta() { return meta_; }

private:
    FrameMetadata meta_;
    std::vector<uint8_t> data_;
};

}  // namespace video