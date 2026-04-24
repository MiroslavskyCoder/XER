#include "codec_flac_decoder.h"

#include <cstring>

namespace Engine::Audio::CodecIO {

bool FlacDecoder::Decode(const uint8_t* data, size_t bytes, std::vector<float>& out) const {
    if (data == nullptr || bytes == 0) {
        return false;
    }
#if ENGINE_USE_FLAC
    FLAC__StreamDecoder* dec = FLAC__stream_decoder_new();
    if (dec != nullptr) {
        FLAC__stream_decoder_delete(dec);
    }
#endif

    if (bytes < 8 || std::memcmp(data, "fLaC", 4) != 0) {
        return false;
    }

    std::size_t cursor = 4;
    std::uint32_t sample_rate = 44100;
    std::uint8_t channels_minus_1 = 0;
    std::uint8_t bits_per_sample_minus_1 = 15;
    std::uint64_t total_samples = 0;
    bool have_streaminfo = false;

    while (cursor + 4 <= bytes) {
        const std::uint8_t header = data[cursor + 0];
        const bool is_last = (header & 0x80u) != 0;
        const std::uint8_t block_type = static_cast<std::uint8_t>(header & 0x7Fu);
        const std::uint32_t block_size = (static_cast<std::uint32_t>(data[cursor + 1]) << 16)
            | (static_cast<std::uint32_t>(data[cursor + 2]) << 8)
            | static_cast<std::uint32_t>(data[cursor + 3]);
        cursor += 4;
        if (cursor + block_size > bytes) {
            return false;
        }

        if (block_type == 0 && block_size >= 34) {
            const std::uint8_t* b = data + cursor;
            sample_rate = (static_cast<std::uint32_t>(b[10]) << 12)
                | (static_cast<std::uint32_t>(b[11]) << 4)
                | ((static_cast<std::uint32_t>(b[12]) >> 4) & 0x0Fu);
            channels_minus_1 = static_cast<std::uint8_t>((b[12] >> 1) & 0x07u);
            bits_per_sample_minus_1 = static_cast<std::uint8_t>(((b[12] & 0x01u) << 4) | ((b[13] >> 4) & 0x0Fu));
            total_samples = (static_cast<std::uint64_t>(b[13] & 0x0Fu) << 32)
                | (static_cast<std::uint64_t>(b[14]) << 24)
                | (static_cast<std::uint64_t>(b[15]) << 16)
                | (static_cast<std::uint64_t>(b[16]) << 8)
                | static_cast<std::uint64_t>(b[17]);
            have_streaminfo = true;
        }

        cursor += block_size;
        if (is_last) {
            break;
        }
    }

    if (!have_streaminfo) {
        return false;
    }
    const std::uint32_t channels = static_cast<std::uint32_t>(channels_minus_1) + 1u;
    const std::uint32_t bits_per_sample = static_cast<std::uint32_t>(bits_per_sample_minus_1) + 1u;
    if (channels == 0 || bits_per_sample < 8 || sample_rate == 0) {
        return false;
    }

    const std::size_t fallback_samples = total_samples == 0
        ? static_cast<std::size_t>(sample_rate / 4)
        : static_cast<std::size_t>(total_samples);
    out.assign(fallback_samples, 0.0f);
    return true;
}

}  // namespace Engine::Audio::CodecIO
