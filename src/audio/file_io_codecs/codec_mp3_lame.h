#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
 
#include <lame/lame.h> 

namespace Engine::Audio::CodecIO {

class Mp3LameCodec {
public:
    bool Encode(const float* input, size_t frames, std::vector<uint8_t>& out) const;
    bool Decode(const uint8_t* data, size_t bytes, std::vector<float>& out) const;
};

}  // namespace Engine::Audio::CodecIO
