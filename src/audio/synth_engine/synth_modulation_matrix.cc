#include "synth_modulation_matrix.h"

namespace Engine::Audio::Synth {

void SynthModulationMatrix::SetAmount(uint32_t source, uint32_t destination, float amount) {
    const uint64_t key = (static_cast<uint64_t>(source) << 32) | destination;
    routes_[key] = amount;
}

float SynthModulationMatrix::GetAmount(uint32_t source, uint32_t destination) const {
    const uint64_t key = (static_cast<uint64_t>(source) << 32) | destination;
    const auto it = routes_.find(key);
    if (it == routes_.end()) {
        return 0.0f;
    }
    return it->second;
}

}  // namespace Engine::Audio::Synth
