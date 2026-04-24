#pragma once

namespace Engine::Audio::Synth {

class SynthOscNoise {
public:
    void SetSeed(unsigned int seed);
    float Process();

private:
    unsigned int state_ = 0x12345678u;
};

}  // namespace Engine::Audio::Synth
