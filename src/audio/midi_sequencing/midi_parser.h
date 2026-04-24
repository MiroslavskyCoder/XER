#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "async_io/log_and_debug/io_dump_helper.h"

namespace Engine::Audio::MIDI {

struct MidiEvent {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    uint32_t tick;
};

class MidiParser {
public:
    bool ParseStream(const uint8_t* data, size_t bytes, std::vector<MidiEvent>& out) const;
};

}  // namespace Engine::Audio::MIDI
