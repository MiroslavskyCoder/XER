#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
 
#include <FLAC/stream_decoder.h> 

namespace Engine::Audio::CodecIO {

class FlacDecoder {
public:
    bool Decode(const uint8_t* data, size_t bytes, std::vector<float>& out) const;
};

}  // namespace Engine::Audio::CodecIO
