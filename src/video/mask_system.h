#pragma once
#include "frame.h"
#include "mask_processor.h"
#include "mask_points_vector.h"
#include "video_constants.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace video {

/// Manages named mask layers and applies them to frames.
class MaskSystem {
public:
    MaskSystem() = default;

    void AddMask(const std::string& name, MaskPointsVector mask);
    void RemoveMask(const std::string& name);
    void Clear();
    bool HasMask(const std::string& name) const;

    /// Apply a named mask to the frame.
    void ApplyMask(Frame& frame, const std::string& name,
                   MaskAction action = MaskAction::CUT_OUTSIDE,
                   uint8_t fill_r = 0, uint8_t fill_g = 0,
                   uint8_t fill_b = 0, uint8_t fill_a = 0);

    /// Apply all masks sequentially.
    void ApplyAll(Frame& frame, MaskAction action = MaskAction::CUT_OUTSIDE);

private:
    std::unordered_map<std::string, MaskPointsVector> masks_;
    MaskProcessor processor_;
};

}  // namespace video