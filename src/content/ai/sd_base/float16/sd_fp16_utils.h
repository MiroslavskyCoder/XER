#pragma once

#include <cstdint>

namespace Engine::AI::SDBase {

class SdFp16Utils {
public:
    static uint16_t Float32ToFp16Bits(float value);
    static float Fp16BitsToFloat32(uint16_t bits);
};

}  // namespace Engine::AI::SDBase
