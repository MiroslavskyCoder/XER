#pragma once
#include "frame.h"
#include <memory>

namespace video {

/// Abstract consumer of rendered frames (encoder, display, file writer).
class FrameSink {
public:
    virtual ~FrameSink() = default;

    virtual bool Open()  = 0;
    virtual void Close() = 0;
    virtual bool IsReady() const = 0;

    /// Push frame downstream. Returns false if dropped.
    virtual bool PushFrame(std::shared_ptr<Frame> frame) = 0;
};

}  // namespace video