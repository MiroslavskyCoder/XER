#include "mask_system.h"

namespace video {

void MaskSystem::AddMask(const std::string& name, MaskPointsVector mask) {
    masks_[name] = std::move(mask);
}

void MaskSystem::RemoveMask(const std::string& name) { masks_.erase(name); }
void MaskSystem::Clear()                              { masks_.clear(); }
bool MaskSystem::HasMask(const std::string& name) const { return masks_.count(name) > 0; }

void MaskSystem::ApplyMask(Frame& frame, const std::string& name,
                            MaskAction action,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    auto it = masks_.find(name);
    if (it == masks_.end()) return;
    processor_.Apply(frame, it->second, action, r, g, b, a);
}

void MaskSystem::ApplyAll(Frame& frame, MaskAction action) {
    for (auto& [name, mask] : masks_)
        processor_.Apply(frame, mask, action);
}

}  // namespace video