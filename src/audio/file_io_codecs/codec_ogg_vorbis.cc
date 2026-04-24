#include "codec_ogg_vorbis.h"

namespace Engine::Audio::CodecIO {

bool OggVorbisCodec::Encode(const float* input, size_t frames, std::vector<uint8_t>& out) const {
    if (input == nullptr || frames == 0) {
        return false;
    }
    out.assign(frames, 0x4F);
    return true;
}

bool OggVorbisCodec::Decode(const uint8_t* data, size_t bytes, std::vector<float>& out) const {
    if (data == nullptr || bytes == 0) {
        return false;
    }
    out.assign(bytes, 0.0f);
    return true;
}

}  // namespace Engine::Audio::CodecIO
