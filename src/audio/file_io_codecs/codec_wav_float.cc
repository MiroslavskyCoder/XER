#include "codec_wav_float.h"

#include <cstring>

namespace Engine::Audio::CodecIO {

bool WavFloatCodec::Encode32(const float* input, size_t frames, std::vector<uint8_t>& out) const {
    if (input == nullptr || frames == 0) {
        return false;
    }

    out.resize(frames * sizeof(float));
    std::memcpy(out.data(), input, out.size());
    return true;
}

bool WavFloatCodec::Decode32(const uint8_t* data, size_t bytes, std::vector<float>& out) const {
    if (data == nullptr || bytes < sizeof(float) || (bytes % sizeof(float)) != 0) {
        return false;
    }

    const size_t samples = bytes / sizeof(float);
    out.resize(samples);
    std::memcpy(out.data(), data, bytes);
    return true;
}

}  // namespace Engine::Audio::CodecIO
