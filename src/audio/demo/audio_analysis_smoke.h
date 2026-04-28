#pragma once

#include <filesystem>
#include <string>

namespace Engine::Audio::Demo {

struct AudioAnalysisSmokeOptions {
	std::filesystem::path input_path;
	std::filesystem::path output_dir;
	int raw_sample_rate = 44100;
	int target_sample_rate = 44100;
};

bool RunAudioAnalysisSmoke(
	const AudioAnalysisSmokeOptions& options,
	std::string* report_out,
	std::string* error_out = nullptr);

}  // namespace Engine::Audio::Demo