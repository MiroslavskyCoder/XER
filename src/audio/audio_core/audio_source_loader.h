#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "audio_sample_rate_converter.h"

namespace Engine::Audio::Core {

struct AudioSourceLoadOptions {
	std::filesystem::path input_path;
	int raw_sample_rate = 44100;
	int target_sample_rate = 44100;
	int target_channels = 1;
	ResampleQuality resample_quality = ResampleQuality::HIGH;
	bool strict_mp3_input = false;
};

struct AudioSourceBuffer {
	std::vector<float> samples;
	int sample_rate = 0;
	int channels = 0;
	int original_sample_rate = 0;
	int original_channels = 0;
	size_t frame_count = 0;
	size_t original_frame_count = 0;
	std::string source_format;
	std::string codec_name;
	std::string decode_backend;
};

class AudioSourceLoader {
public:
	static bool Load(
		const AudioSourceLoadOptions& options,
		AudioSourceBuffer* output,
		std::string* error_out = nullptr);
};

}  // namespace Engine::Audio::Core