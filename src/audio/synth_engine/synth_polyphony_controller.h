#pragma once

#include <cstdint>
#include <vector>

namespace Engine::Audio::Synth {

class SynthPolyphonyController {
public:
    void SetMaxVoices(uint32_t max_voices);
    int AllocateVoice(uint8_t note);
    void ReleaseVoice(uint8_t note);

private:
    uint32_t max_voices_ = 16;
    std::vector<uint8_t> active_notes_;
};

}  // namespace Engine::Audio::Synth
