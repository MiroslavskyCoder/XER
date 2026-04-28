#pragma once

#include <filesystem>
#include <string>

namespace Engine::Audio::Demo {

struct AudioFxCustomOptions {
	std::filesystem::path output_dir;
	std::filesystem::path input_path;
	std::string effect_name;
	int raw_sample_rate = 44100;
	int target_sample_rate = 44100;
	int target_channels = -1;
};

bool RunAudioFxCustom(
	const AudioFxCustomOptions& options,
	std::string* report_out,
	std::string* error_out = nullptr);

}  // namespace Engine::Audio::Demo