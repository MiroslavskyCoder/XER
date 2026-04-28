#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Engine::Audio::CodecIO::detail {

bool DecodeAudioBufferWithFfmpeg(
	const uint8_t* data,
	size_t bytes,
	const std::string& extension,
	std::vector<float>* output,
	std::string* error_out = nullptr);

}  // namespace Engine::Audio::CodecIO::detail