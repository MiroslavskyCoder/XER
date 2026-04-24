#pragma once

#include <cstdint>
 
#include <absl/container/flat_hash_map.h> 

namespace Engine::Audio::Synth {

class SynthModulationMatrix {
public:
    void SetAmount(uint32_t source, uint32_t destination, float amount);
    float GetAmount(uint32_t source, uint32_t destination) const;

private: 
    absl::flat_hash_map<uint64_t, float> routes_; 
};

}  // namespace Engine::Audio::Synth
