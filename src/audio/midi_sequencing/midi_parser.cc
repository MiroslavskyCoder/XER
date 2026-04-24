#include "midi_parser.h"

namespace Engine::Audio::MIDI {

bool MidiParser::ParseStream(const uint8_t* data, size_t bytes, std::vector<MidiEvent>& out) const {
    if (data == nullptr || bytes < 3) {
        return false;
    }

    out.clear();
    for (size_t i = 0; i + 2 < bytes; i += 3) {
        out.push_back(MidiEvent{data[i], data[i + 1], data[i + 2], static_cast<uint32_t>(i / 3)});
    }
    return !out.empty();
}

}  // namespace Engine::Audio::MIDI
