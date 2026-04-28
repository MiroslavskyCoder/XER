#include "codec_aac_adts.h"

#include "codec_ffmpeg_decode_helper.h"

#include <algorithm>
#include <cstdint>

namespace Engine::Audio::CodecIO {

namespace {

void WriteAdtsHeader(std::uint8_t* h, std::uint16_t frame_length) {
    const std::uint8_t profile = 1;      // AAC LC -> profile index in ADTS is 1
    const std::uint8_t sf_index = 4;     // 44.1kHz
    const std::uint8_t channel_cfg = 1;  // mono

    h[0] = 0xFF;
    h[1] = 0xF1;
    h[2] = static_cast<std::uint8_t>(((profile & 0x03u) << 6) | ((sf_index & 0x0Fu) << 2) | ((channel_cfg >> 2) & 0x01u));
    h[3] = static_cast<std::uint8_t>(((channel_cfg & 0x03u) << 6) | ((frame_length >> 11) & 0x03u));
    h[4] = static_cast<std::uint8_t>((frame_length >> 3) & 0xFFu);
    h[5] = static_cast<std::uint8_t>(((frame_length & 0x07u) << 5) | 0x1Fu);
    h[6] = 0xFC;
}

} // namespace

bool AacAdtsCodec::Encode(const float* input, size_t frames, std::vector<uint8_t>& out) const {
    if (input == nullptr || frames == 0) {
        return false;
    }
	(void)input;
	(void)frames;
	out.clear();
	return false;
}

bool AacAdtsCodec::Decode(const uint8_t* data, size_t bytes, std::vector<float>& out) const {
    if (data == nullptr || bytes == 0) {
        return false;
    }

	std::string error;
	if (detail::DecodeAudioBufferWithFfmpeg(data, bytes, ".aac", &out, &error)) {
		return true;
	}

    std::vector<float> decoded;
    std::size_t cursor = 0;
    while (cursor + 7 <= bytes) {
        if (data[cursor] != 0xFF || (data[cursor + 1] & 0xF0u) != 0xF0u) {
            return false;
        }
        const std::uint16_t frame_length = static_cast<std::uint16_t>(((data[cursor + 3] & 0x03u) << 11)
            | (data[cursor + 4] << 3)
            | ((data[cursor + 5] >> 5) & 0x07u));
        if (frame_length < 7 || cursor + frame_length > bytes) {
            return false;
        }

        const std::size_t payload_bytes = frame_length - 7;
        if ((payload_bytes % 2) != 0) {
            return false;
        }
        const std::size_t samples = payload_bytes / 2;
        const std::size_t payload_offset = cursor + 7;
        decoded.reserve(decoded.size() + samples);
        for (std::size_t i = 0; i < samples; ++i) {
            const std::int16_t s = static_cast<std::int16_t>(data[payload_offset + i * 2 + 0] | (data[payload_offset + i * 2 + 1] << 8));
            decoded.push_back(static_cast<float>(s) / 32768.0f);
        }
        cursor += frame_length;
    }
    if (decoded.empty()) {
        return false;
    }
    out = std::move(decoded);
    return true;
}

}  // namespace Engine::Audio::CodecIO
