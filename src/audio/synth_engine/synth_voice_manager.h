#pragma once

#include <cstdint>
 
#include <absl/container/flat_hash_map.h> 

#include "synth_env_adsr.h"
#include "synth_osc_analog.h"

namespace Engine::Audio::Synth {

struct VoiceState {
    uint8_t note = 0;
    bool active = false;
    SynthOscAnalog osc;
    SynthEnvAdsr env;
};

class SynthVoiceManager {
public:
    void NoteOn(uint8_t note, float freq_hz);
    void NoteOff(uint8_t note);
    float RenderSample(float sample_rate);

private: 
    absl::flat_hash_map<uint8_t, VoiceState> voices_; 
};

}  // namespace Engine::Audio::Synth
