#include "custom_effect_node_wrapper.h"

#include <algorithm>
#include <optional>

#include "custom_effect_core.h"
#include "custom_effect_fxdata.h"

namespace Engine::Audio::FX {

namespace {

std::optional<uint32_t> ResolveClapParameterId(
	const std::vector<Plugin::PluginParameterInfo>& infos,
	const std::string& name) {
	for (const auto& info : infos) {
		if (info.name == name) {
			return info.id;
		}
	}
	try {
		return static_cast<uint32_t>(std::stoul(name));
	} catch (...) {
		return std::nullopt;
	}
}

}  // namespace

bool CustomEffectNodeWrapper::Initialize(const CustomEffectNode& node, float sample_rate, size_t max_block_size, std::string* error_out) {
	if (!ValidateCustomEffectNode(node, error_out) || sample_rate <= 0.0f || max_block_size == 0) {
		return false;
	}

	node_ = node;
	sample_rate_ = sample_rate;
	max_block_size_ = max_block_size;
	wet_buffer_.assign(max_block_size_, 0.0f);
	if (node_.backend == CustomEffectBackend::kClapPlugin) {
		if (!clap_host_.Initialize(sample_rate_, static_cast<uint32_t>(max_block_size_))
			|| !clap_host_.LoadPlugin(node_.plugin_reference)) {
			if (error_out != nullptr) {
				*error_out = "failed to initialize CLAP custom effect node: " + node_.label;
			}
			return false;
		}
		const auto parameter_infos = clap_host_.GetParameters();
		for (const auto& parameter : node_.parameters) {
			const auto parameter_id = ResolveClapParameterId(parameter_infos, parameter.name);
			if (!parameter_id.has_value()) {
				continue;
			}
			clap_host_.SetParameter(*parameter_id, parameter.value);
		}
		return true;
	}

	switch (node_.algorithm) {
	case CustomEffectAlgorithm::kPassthrough:
	case CustomEffectAlgorithm::kCompressor:
	case CustomEffectAlgorithm::kLimiter:
		return true;
	case CustomEffectAlgorithm::kChorus:
		chorus_.SetRateHz(GetCustomEffectParameterValue(node_.parameters, "rate_hz", 0.8f));
		chorus_.SetDepthSamples(GetCustomEffectParameterValue(node_.parameters, "depth_samples", 14.0f));
		chorus_.SetMix(1.0f);
		return chorus_.Initialize(sample_rate_, std::max<size_t>(4096u, max_block_size_ * 2));
	case CustomEffectAlgorithm::kParametricEq:
		if (!parametric_eq_.Initialize(sample_rate_, 1u)) {
			return false;
		}
		return parametric_eq_.SetBand(0u, ParametricBand{
			GetCustomEffectParameterValue(node_.parameters, "frequency_hz", 2200.0f),
			GetCustomEffectParameterValue(node_.parameters, "q", 0.8f),
			GetCustomEffectParameterValue(node_.parameters, "gain_db", 0.0f),
			true,
		});
	case CustomEffectAlgorithm::kPlugin:
		return false;
	case CustomEffectAlgorithm::kUnknown:
		break;
	}
	if (error_out != nullptr) {
		*error_out = "unsupported custom effect node algorithm";
	}
	return false;
}

bool CustomEffectNodeWrapper::ProcessBlock(
	const float* input,
	size_t frame_count,
	size_t frame_offset,
	float* output,
	std::string* error_out) {
	if (input == nullptr || output == nullptr || frame_count == 0 || frame_count > max_block_size_) {
		if (error_out != nullptr) {
			*error_out = "invalid custom effect processing block";
		}
		return false;
	}

	if (!node_.enabled) {
		std::copy(input, input + static_cast<std::ptrdiff_t>(frame_count), output);
		return true;
	}

	bool processed = false;
	if (node_.backend == CustomEffectBackend::kClapPlugin) {
		processed = clap_host_.Process(input, wet_buffer_.data(), static_cast<uint32_t>(frame_count));
	} else {
	switch (node_.algorithm) {
	case CustomEffectAlgorithm::kPassthrough:
		std::copy(input, input + static_cast<std::ptrdiff_t>(frame_count), wet_buffer_.begin());
		processed = true;
		break;
	case CustomEffectAlgorithm::kCompressor:
		compressor_.SetThresholdDb(GetCustomEffectParameterValue(node_.parameters, "threshold_db", -16.0f));
		compressor_.SetRatio(GetCustomEffectParameterValue(node_.parameters, "ratio", 3.0f));
		processed = compressor_.ProcessBlock(input, frame_count, wet_buffer_.data());
		break;
	case CustomEffectAlgorithm::kLimiter:
		limiter_.SetCeilingDb(GetCustomEffectParameterValue(node_.parameters, "ceiling_db", -1.0f));
		limiter_.SetRelease(GetCustomEffectParameterValue(node_.parameters, "release", 0.002f));
		processed = limiter_.ProcessBlock(input, frame_count, wet_buffer_.data());
		break;
	case CustomEffectAlgorithm::kChorus:
		processed = chorus_.ProcessBlock(input, frame_count, wet_buffer_.data());
		break;
	case CustomEffectAlgorithm::kParametricEq:
		processed = parametric_eq_.ProcessBlock(input, frame_count, wet_buffer_.data());
		break;
	case CustomEffectAlgorithm::kPlugin:
		break;
	case CustomEffectAlgorithm::kUnknown:
		break;
	}
	}
	if (!processed) {
		if (error_out != nullptr) {
			*error_out = "custom effect node block processing failed: " + node_.label;
		}
		return false;
	}

	for (size_t index = 0; index < frame_count; ++index) {
		const float mix = ResolveCustomEffectNodeMix(node_, frame_offset + index);
		output[index] = MixCustomEffectDryWet(input[index], wet_buffer_[index], mix);
	}
	return true;
}

std::string CustomEffectNodeWrapper::GetReport() const {
	if (node_.backend == CustomEffectBackend::kClapPlugin) {
		return node_.label + ":clap=" + clap_host_.GetLoadedPluginId();
	}
	switch (node_.algorithm) {
	case CustomEffectAlgorithm::kCompressor:
		return node_.label + ":" + compressor_.GetReport();
	case CustomEffectAlgorithm::kLimiter:
		return node_.label + ":" + limiter_.GetReport();
	case CustomEffectAlgorithm::kChorus:
		return node_.label + ":" + chorus_.GetReport();
	case CustomEffectAlgorithm::kParametricEq:
		return node_.label + ":" + parametric_eq_.GetReport();
	case CustomEffectAlgorithm::kPassthrough:
		return node_.label + ":passthrough";
	case CustomEffectAlgorithm::kPlugin:
		return node_.label + ":plugin";
	case CustomEffectAlgorithm::kUnknown:
		break;
	}
	return node_.label + ":unknown";
}

}  // namespace Engine::Audio::FX
