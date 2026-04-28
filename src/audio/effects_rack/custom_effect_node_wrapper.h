#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "custom_effect_node.h"
#include "audio/plugin_wrappers/clap_host_interface.h"
#include "fx_dynamics_compressor.h"
#include "fx_dynamics_limiter.h"
#include "fx_eq_parametric.h"
#include "fx_mod_chorus.h"

namespace Engine::Audio::FX {

class CustomEffectNodeWrapper {
public:
	bool Initialize(const CustomEffectNode& node, float sample_rate, size_t max_block_size, std::string* error_out = nullptr);
	bool ProcessBlock(
		const float* input,
		size_t frame_count,
		size_t frame_offset,
		float* output,
		std::string* error_out = nullptr);
	std::string GetReport() const;
	const CustomEffectNode& GetNode() const { return node_; }

private:
	CustomEffectNode node_;
	float sample_rate_ = 44100.0f;
	size_t max_block_size_ = 0;
	Plugin::ClapHostInterface clap_host_;
	DynamicsCompressor compressor_;
	DynamicsLimiter limiter_;
	ModChorus chorus_;
	ParametricEQ parametric_eq_;
	std::vector<float> wet_buffer_;
};

}  // namespace Engine::Audio::FX
