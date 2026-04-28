#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include "custom_effect_processor.h"
#include "custom_effect_struct.h"

namespace Engine::Audio::FX {

bool ApplyCustomEffectProcessors(
	std::deque<CustomEffectProcessor>* processors,
	const std::vector<float>& input,
	const CustomEffectRenderConfig& render_config,
	std::vector<float>* output,
	size_t* worker_count_used = nullptr,
	std::string* error_out = nullptr);

}  // namespace Engine::Audio::FX
