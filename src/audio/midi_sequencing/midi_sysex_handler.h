#pragma once

#include <cstdint>
#include <vector>

#include "async_io/log_and_debug/io_dump_helper.h"

namespace Engine::Audio::MIDI {

class MidiSysexHandler {
public:
    bool IsValidSysex(const std::vector<uint8_t>& msg) const;
    std::vector<uint8_t> BuildIdentityRequest() const;
};

}  // namespace Engine::Audio::MIDI
