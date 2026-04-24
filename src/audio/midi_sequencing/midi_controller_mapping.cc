#include "midi_controller_mapping.h"

namespace Engine::Audio::MIDI {

void MidiControllerMapping::Bind(uint8_t cc, uint32_t param_id) {
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    map_[cc] = param_id;
}

bool MidiControllerMapping::Resolve(uint8_t cc, uint32_t& param_id) const {
    AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    const auto it = map_.find(cc);
    if (it == map_.end()) {
        return false;
    }
    param_id = it->second;
    return true;
}

}  // namespace Engine::Audio::MIDI
