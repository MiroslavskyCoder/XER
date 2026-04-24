#include "synth_polyphony_controller.h"

#include <algorithm>

namespace Engine::Audio::Synth {

void SynthPolyphonyController::SetMaxVoices(uint32_t max_voices) {
    max_voices_ = std::max<uint32_t>(1, max_voices);
}

int SynthPolyphonyController::AllocateVoice(uint8_t note) {
    if (active_notes_.size() >= max_voices_) {
        active_notes_.erase(active_notes_.begin());
    }
    active_notes_.push_back(note);
    return static_cast<int>(active_notes_.size() - 1);
}

void SynthPolyphonyController::ReleaseVoice(uint8_t note) {
    const auto it = std::find(active_notes_.begin(), active_notes_.end(), note);
    if (it != active_notes_.end()) {
        active_notes_.erase(it);
    }
}

}  // namespace AIToolsXPro::Audio::Synth
