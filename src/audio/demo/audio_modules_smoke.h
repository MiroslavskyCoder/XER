#pragma once

#include <filesystem>
#include <string>

namespace Engine::Audio::Demo {

struct AudioModulesSmokeOptions {
	std::filesystem::path input_path;
	std::filesystem::path output_dir;
};

bool RunAudioModulesSmoke(
	const AudioModulesSmokeOptions& options,
	std::string* report,
	std::string* error_out = nullptr);

}  // namespace Engine::Audio::Demo