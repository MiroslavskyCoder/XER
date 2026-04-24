#pragma once

#include <vector>

#include "async_io/async_task_queue.h"
#include "midi_parser.h"

namespace Engine::Audio::MIDI {

class MidiPatternSequencer {
public:
    void SetPattern(const std::vector<MidiEvent>& pattern);
    std::vector<MidiEvent> CollectRange(uint32_t start_tick, uint32_t end_tick) const;

private:
    std::vector<MidiEvent> pattern_;
};

}  // namespace Engine::Audio::MIDI
