#include "midi_event_dispatcher.h"

namespace Engine::Audio::MIDI {

void MidiEventDispatcher::AddListener(const Callback& cb) {
    IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    listeners_.push_back(cb);
}

void MidiEventDispatcher::Dispatch(const MidiEvent& event) {
    IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
    for (const auto& cb : listeners_) {
        cb(event);
    }
}

}  // namespace Engine::Audio::MIDI
