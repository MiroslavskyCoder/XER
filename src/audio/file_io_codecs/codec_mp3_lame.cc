#include "codec_mp3_lame.h"

#include "codec_ffmpeg_decode_helper.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace Engine::Audio::CodecIO {

namespace {

constexpr std::uint8_t kFallbackMagic[4] = {'M', 'P', 'F', '0'};

} // namespace

bool Mp3LameCodec::Encode(const float* input, size_t frames, std::vector<uint8_t>& out) const {
    if (input == nullptr || frames == 0) {
        return false;
    }

#if ENGINE_CODEC_HAS_LAME_HEADERS
    lame_t lame = lame_init();
    if (lame == nullptr) {
        return false;
    }
    lame_set_in_samplerate(lame, 44100);
    lame_set_num_channels(lame, 1);
    lame_set_quality(lame, 2);
    if (lame_init_params(lame) < 0) {
        lame_close(lame);
        return false;
    }

    std::vector<short> pcm(frames);
    for (size_t i = 0; i < frames; ++i) {
        const float clamped = std::clamp(input[i], -1.0f, 1.0f);
        pcm[i] = static_cast<short>(clamped * 32767.0f);
    }

    const int mp3_capacity = static_cast<int>(1.25 * frames + 7200);
    out.assign(static_cast<std::size_t>(mp3_capacity), 0);
    const int encoded = lame_encode_buffer(
        lame,
        pcm.data(),
        pcm.data(),
        static_cast<int>(frames),
        out.data(),
        mp3_capacity);
    if (encoded < 0) {
        lame_close(lame);
        out.clear();
        return false;
    }
    const int flushed = lame_encode_flush(lame, out.data() + encoded, mp3_capacity - encoded);
    lame_close(lame);
    if (flushed < 0) {
        out.clear();
        return false;
    }
    out.resize(static_cast<std::size_t>(encoded + flushed));
    return !out.empty();
#else
    (void)input;
    (void)frames;
    out.clear();
    return false;
#endif
}

bool Mp3LameCodec::Decode(const uint8_t* data, size_t bytes, std::vector<float>& out) const {
    if (data == nullptr || bytes == 0) {
        return false;
    }

    if (bytes >= 4 && data[0] == kFallbackMagic[0] && data[1] == kFallbackMagic[1]
        && data[2] == kFallbackMagic[2] && data[3] == kFallbackMagic[3]) {
        const std::size_t payload = bytes - 4;
        if ((payload % 2) != 0) {
            return false;
        }
        const std::size_t samples = payload / 2;
        out.resize(samples);
        for (std::size_t i = 0; i < samples; ++i) {
            const std::int16_t s = static_cast<std::int16_t>(data[4 + i * 2 + 0] | (data[4 + i * 2 + 1] << 8));
            out[i] = static_cast<float>(s) / 32768.0f;
        }
        return true;
    }

    std::string error;
    if (!detail::DecodeAudioBufferWithFfmpeg(data, bytes, ".mp3", &out, &error)) {
        out.clear();
        return false;
    }
    return true;
}

}  // namespace Engine::Audio::CodecIO
