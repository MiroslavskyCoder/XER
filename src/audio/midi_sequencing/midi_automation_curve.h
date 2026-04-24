#pragma once

#include <cstdint>
#include <vector>

#include "async_io/sync_primitives/mutex_wrapper.h"

namespace Engine::Audio::MIDI {

struct AutomationPoint {
    uint32_t tick;
    float value;
};

class MidiAutomationCurve {
public:
    void AddPoint(uint32_t tick, float value);
    float Evaluate(uint32_t tick) const;

private:
    mutable IO::Sync::MutexWrapper mutex_{"midi_automation_curve"};
    std::vector<AutomationPoint> points_;
};

}  // namespace Engine::Audio::MIDI
