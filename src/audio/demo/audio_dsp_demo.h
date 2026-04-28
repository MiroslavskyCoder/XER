#pragma once

#include <filesystem>
#include <string>

namespace Engine::Audio::Demo {

bool RunAudioDSPDemo(
	const std::filesystem::path& output_dir,
	std::string* report_out,
	std::string* error_out = nullptr);

}  // namespace Engine::Audio::Demo