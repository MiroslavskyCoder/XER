#pragma once

#include <cstdint>
#include <unordered_map>

#include "async_io/sync_primitives/mutex_wrapper.h"

namespace Engine::Audio::MIDI {

class MidiControllerMapping {
public:
    void Bind(uint8_t cc, uint32_t param_id);
    bool Resolve(uint8_t cc, uint32_t& param_id) const;

private:
    mutable AsyncIO::IO::Sync::MutexWrapper mutex_{"midi_cc_map"};
    std::unordered_map<uint8_t, uint32_t> map_;
};

}  // namespace Engine::Audio::MIDI
