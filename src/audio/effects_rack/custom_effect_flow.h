#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "custom_effect_apply_multi.h"
#include "custom_effect_package.h"

namespace Engine::Audio::FX {

class CustomEffectFlow {
public:
	bool Initialize(const CustomEffectPackage& package, float sample_rate, size_t max_block_size, std::string* error_out = nullptr);
	bool Process(const std::vector<float>& input, std::vector<float>* output, std::string* error_out = nullptr);
	std::string GetReport() const;
	const CustomEffectPackage& GetPackage() const { return package_; }
	const CustomEffectReport& GetLastReport() const { return last_report_; }

private:
	CustomEffectPackage package_;
	CustomEffectRenderConfig render_config_;
	std::vector<uint32_t> stage_indices_;
	std::vector<std::deque<CustomEffectProcessor>> stage_processors_;
	CustomEffectReport last_report_;
};

}  // namespace Engine::Audio::FX
