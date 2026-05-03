#pragma once
#include "frame.h"
#include <memory>

namespace video {

/// Abstract source of decoded frames (camera, file, network).
class FrameSource {
public:
    virtual ~FrameSource() = default;

    virtual bool Open(const std::string& uri) = 0;
    virtual void Close() = 0;
    virtual bool IsOpen() const = 0;

    /// Pull next frame. Returns nullptr on EOF or error.
    virtual std::shared_ptr<Frame> NextFrame() = 0;

    virtual int Width()  const = 0;
    virtual int Height() const = 0;
    virtual double Fps() const = 0;
};

}  // namespace video