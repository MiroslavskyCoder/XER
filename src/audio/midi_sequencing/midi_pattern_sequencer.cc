#include "midi_pattern_sequencer.h"

namespace Engine::Audio::MIDI {

void MidiPatternSequencer::SetPattern(const std::vector<MidiEvent>& pattern) {
    pattern_ = pattern;
}

std::vector<MidiEvent> MidiPatternSequencer::CollectRange(uint32_t start_tick, uint32_t end_tick) const {
    std::vector<MidiEvent> out;
    for (const auto& e : pattern_) {
        if (e.tick >= start_tick && e.tick < end_tick) {
            out.push_back(e);
        }
    }
    return out;
}

}  // namespace Engine::Audio::MIDI
