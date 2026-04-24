#include "synth_voice_manager.h"

namespace Engine::Audio::Synth {

void SynthVoiceManager::NoteOn(uint8_t note, float freq_hz) {
    auto& v = voices_[note];
    v.note = note;
    v.active = true;
    v.osc.SetFrequency(freq_hz);
    v.env.NoteOn();
}

void SynthVoiceManager::NoteOff(uint8_t note) {
    auto it = voices_.find(note);
    if (it != voices_.end()) {
        it->second.env.NoteOff();
    }
}

float SynthVoiceManager::RenderSample(float sample_rate) {
    float out = 0.0f;
    for (auto& kv : voices_) {
        auto& v = kv.second;
        const float env = v.env.Process(sample_rate);
        const float osc = v.osc.Process(sample_rate);
        out += osc * env;
    }
    return out;
}

}  // namespace Engine::Audio::Synth
