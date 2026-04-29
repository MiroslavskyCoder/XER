#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Engine::Audio::Demo {

struct AudioFxCustomOptions {
	std::filesystem::path output_dir;
	std::filesystem::path input_path;
	std::string effect_name;
	std::vector<std::string> effect_names;
	int raw_sample_rate = 44100;
	int target_sample_rate = 44100;
	int target_channels = -1;
	std::string batch_mode = "parallel";
	bool json_summary = false;
};

bool RunAudioFxCustom(
	const AudioFxCustomOptions& options,
	std::string* report_out,
	std::string* error_out = nullptr);

bool RunAudioFxBatch(
	const AudioFxCustomOptions& options,
	std::string* report_out,
	std::string* error_out = nullptr);

}  // namespace Engine::Audio::Demo