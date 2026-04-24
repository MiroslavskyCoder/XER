#include "midi_sysex_handler.h"

namespace Engine::Audio::MIDI {

bool MidiSysexHandler::IsValidSysex(const std::vector<uint8_t>& msg) const {
    return msg.size() >= 2 && msg.front() == 0xF0 && msg.back() == 0xF7;
}

std::vector<uint8_t> MidiSysexHandler::BuildIdentityRequest() const {
    return {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};
}

}  // namespace Engine::Audio::MIDI
