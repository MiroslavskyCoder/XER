#include "fx_customs_presets.h"

#include <algorithm>
#include <cctype>
#include <cmath>

#include "audio/audio_core/audio_interleave_processor.h"
#include "audio/effects_rack/custom_effect_algorithms.h"
#include "audio/effects_rack/custom_effect_core.h"
#include "audio/effects_rack/custom_effect_fxdata.h"
#include "audio/effects_rack/custom_effect_invoke.h"

namespace Engine::Audio::FX::Customs {

namespace {

std::string NormalizePresetName(std::string name) {
	std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
	return name;
}

CustomEffectPackage BuildNamedPreset(const std::string& name, const std::string& clap_plugin_reference = "builtin://gain") {
	return BuildNamedCustomEffectPackage(name, clap_plugin_reference);
}

float ChannelPan(int channel_index, int channel_count) {
	if (channel_count <= 1) {
		return 0.0f;
	}
	return -1.0f + 2.0f * static_cast<float>(channel_index) / static_cast<float>(channel_count - 1);
}

bool IsSpatialPreset(const std::string& normalized_name) {
	return normalized_name == "stereoconverter"
		|| normalized_name == "superreverb"
		|| normalized_name == "roomreverb"
		|| normalized_name == "studioreverb"
		|| normalized_name == "eqreverb"
		|| normalized_name == "delayreverb"
		|| normalized_name == "delay"
		|| normalized_name == "airvoice";
}

void SetNodeMix(CustomEffectPackage* package, const std::string& label, float mix) {
	if (package == nullptr) {
		return;
	}
	for (auto& node : package->nodes) {
		if (node.label == label) {
			node.mix_curve = {CustomEffectCurvePoint{0u, mix}};
			return;
		}
	}
}

void ApplySpatialVariant(CustomEffectPackage* package, const std::string& normalized_name, int channel_index, int channel_count) {
	if (package == nullptr || channel_count <= 1) {
		return;
	}

	const float pan = ChannelPan(channel_index, channel_count);
	const float pan_abs = std::abs(pan);
	if (normalized_name == "stereoconverter") {
		InvokeCustomEffectParameter(package, "stereo_width_chorus", "rate_hz", 0.70f + 0.16f * pan);
		InvokeCustomEffectParameter(package, "stereo_width_chorus", "depth_samples", 18.0f + 6.0f * (1.0f + pan_abs));
		InvokeCustomEffectParameter(package, "stereo_width_delay", "delay_samples", 72.0f + 48.0f * static_cast<float>(channel_index + 1));
		InvokeCustomEffectParameter(package, "stereo_width_delay", "feedback", 0.10f + 0.04f * pan_abs);
		SetNodeMix(package, "stereo_width_delay", 0.18f + 0.08f * pan_abs);
		return;
	}

	if (normalized_name == "superreverb" || normalized_name == "roomreverb" || normalized_name == "studioreverb" || normalized_name == "eqreverb" || normalized_name == "delayreverb") {
		InvokeCustomEffectParameter(package, normalized_name == "eqreverb" ? "eq_reverb_room" : (normalized_name == "delayreverb" ? "delay_reverb_room" : (normalized_name == "superreverb" ? "super_reverb" : (normalized_name == "roomreverb" ? "room_reverb" : "studio_reverb"))), "room_size", std::clamp(0.45f + 0.22f * (1.0f - pan_abs) + 0.05f * pan, 0.2f, 0.98f));
		InvokeCustomEffectParameter(package, normalized_name == "eqreverb" ? "eq_reverb_room" : (normalized_name == "delayreverb" ? "delay_reverb_room" : (normalized_name == "superreverb" ? "super_reverb" : (normalized_name == "roomreverb" ? "room_reverb" : "studio_reverb"))), "damping", std::clamp(0.24f + 0.10f * pan_abs, 0.05f, 0.9f));
	}

	if (normalized_name == "delayreverb" || normalized_name == "delay") {
		InvokeCustomEffectParameter(package, normalized_name == "delay" ? "delay_main" : "delay_reverb_delay", "delay_samples", 720.0f + 120.0f * static_cast<float>(channel_index + 1));
		InvokeCustomEffectParameter(package, normalized_name == "delay" ? "delay_main" : "delay_reverb_delay", "feedback", 0.22f + 0.06f * pan_abs);
	}

	if (normalized_name == "airvoice") {
		InvokeCustomEffectParameter(package, "airvoice_air", "frequency_hz", 11000.0f + 800.0f * pan);
		InvokeCustomEffectParameter(package, "airvoice_air", "gain_db", 4.8f + 0.8f * (1.0f - pan_abs));
		for (auto& node : package->nodes) {
			if (node.label == "airvoice_limiter") {
				node.stage_index = 5u;
			}
		}
		CustomEffectNode space = CreateDefaultCustomEffectNode("reverb_algorithmic");
		space.label = "airvoice_space" + std::to_string(channel_index);
		space.stage_index = 4u;
		space.mix_curve = {CustomEffectCurvePoint{0u, 0.18f + 0.06f * pan_abs}};
		SetCustomEffectParameterValue(&space.parameters, "room_size", std::clamp(0.48f + 0.12f * (1.0f - pan_abs), 0.2f, 0.9f));
		SetCustomEffectParameterValue(&space.parameters, "damping", 0.24f + 0.08f * pan_abs);
		package->nodes.push_back(std::move(space));
	}
}

}  // namespace

std::vector<std::string> ListFxCustomPresetNames() {
	return ListAvailableCustomEffects();
}

bool IsFxCustomPresetSupported(const std::string& name) {
	const std::string normalized = NormalizePresetName(name);
	const auto names = ListFxCustomPresetNames();
	return std::any_of(names.begin(), names.end(), [&](const std::string& candidate) {
		return NormalizePresetName(candidate) == normalized;
	});
}

CustomEffectPackage BuildFxCustomPreset(const std::string& name, const std::string& clap_plugin_reference) {
	return BuildNamedPreset(name, clap_plugin_reference);
}

bool RenderFxCustomPreset(
	const std::string& name,
	float sample_rate,
	const std::vector<float>& input,
	std::vector<float>* output,
	CustomEffectReport* report_out,
	const std::string& clap_plugin_reference,
	std::string* error_out) {
	return RunCustomEffectPackage(
		BuildFxCustomPreset(name, clap_plugin_reference),
		sample_rate,
		input,
		output,
		report_out,
		error_out);
}

bool RenderFxCustomPresetInterleaved(
	const std::string& name,
	float sample_rate,
	const std::vector<float>& input,
	int channels,
	std::vector<float>* output,
	CustomEffectReport* report_out,
	const std::string& clap_plugin_reference,
	std::string* error_out) {
	if (output == nullptr || channels <= 0 || input.empty() || (input.size() % static_cast<size_t>(channels)) != 0u) {
		if (error_out != nullptr) {
			*error_out = "fx custom interleaved render input is invalid";
		}
		return false;
	}

	const std::string normalized = NormalizePresetName(name);
	const size_t frame_count = input.size() / static_cast<size_t>(channels);
	std::vector<std::vector<float>> input_channels;
	if (!Engine::Audio::Core::AudioInterleaveProcessor::DeinterleavePlanar(
			input.data(),
			channels,
			frame_count,
			input_channels)) {
		if (error_out != nullptr) {
			*error_out = "failed to deinterleave fx custom input";
		}
		return false;
	}

	std::vector<std::vector<float>> output_channels(static_cast<size_t>(channels));
	CustomEffectReport aggregate_report;
	aggregate_report.label = name;
	aggregate_report.channel_count = static_cast<size_t>(channels);
	aggregate_report.worker_count_used = 1u;
	for (int channel_index = 0; channel_index < channels; ++channel_index) {
		CustomEffectPackage package = BuildFxCustomPreset(name, clap_plugin_reference);
		if (IsSpatialPreset(normalized)) {
			ApplySpatialVariant(&package, normalized, channel_index, channels);
		}

		CustomEffectReport channel_report;
		if (!RunCustomEffectPackage(
				package,
				sample_rate,
				input_channels[static_cast<size_t>(channel_index)],
				&output_channels[static_cast<size_t>(channel_index)],
				&channel_report,
				error_out)) {
			return false;
		}
		aggregate_report.node_count += channel_report.node_count;
		aggregate_report.stage_count = std::max(aggregate_report.stage_count, channel_report.stage_count);
		aggregate_report.worker_count_used = std::max(aggregate_report.worker_count_used, channel_report.worker_count_used);
		for (const auto& stage_report : channel_report.stage_reports) {
			aggregate_report.stage_reports.push_back("channel=" + std::to_string(channel_index) + "," + stage_report);
		}
		for (const auto& node_report : channel_report.node_reports) {
			aggregate_report.node_reports.push_back("channel=" + std::to_string(channel_index) + "," + node_report);
		}
	}

	output->assign(frame_count * static_cast<size_t>(channels), 0.0f);
	std::vector<const float*> channel_ptrs(static_cast<size_t>(channels), nullptr);
	for (int channel_index = 0; channel_index < channels; ++channel_index) {
		channel_ptrs[static_cast<size_t>(channel_index)] = output_channels[static_cast<size_t>(channel_index)].data();
	}
	if (!Engine::Audio::Core::AudioInterleaveProcessor::Interleave(
			channel_ptrs.data(),
			channels,
			frame_count,
			output->data())) {
		if (error_out != nullptr) {
			*error_out = "failed to interleave fx custom output";
		}
		return false;
	}
	ComputeCustomEffectReportStats(*output, &aggregate_report);
	if (report_out != nullptr) {
		*report_out = aggregate_report;
	}
	return true;
}

CustomEffectPackage BuildBassBoostEffect() {
	return BuildNamedPreset("BassBoost");
}

CustomEffectPackage BuildEqEffect() {
	return BuildNamedPreset("EQ");
}

CustomEffectPackage BuildHighEffect() {
	return BuildNamedPreset("High");
}

CustomEffectPackage BuildAirHighEffect() {
	return BuildNamedPreset("AIRHigh");
}

CustomEffectPackage BuildCurveAirEffect() {
	return BuildNamedPreset("CurveAIR");
}

CustomEffectPackage BuildDeEsserEffect() {
	return BuildNamedPreset("DeEsser");
}

CustomEffectPackage BuildCompressorEffect() {
	return BuildNamedPreset("Compressor");
}

CustomEffectPackage BuildLimiterEffect() {
	return BuildNamedPreset("Limiter");
}

CustomEffectPackage BuildAutoTuneEffect() {
	return BuildNamedPreset("AutoTune");
}

CustomEffectPackage BuildPitchKeyEffect() {
	return BuildNamedPreset("PitchKey");
}

CustomEffectPackage BuildAutoPitchEffect() {
	return BuildNamedPreset("AutoPitch");
}

CustomEffectPackage BuildStereoConverterEffect() {
	return BuildNamedPreset("Stereo Converter");
}

CustomEffectPackage BuildVhsEffect() {
	return BuildNamedPreset("VHS");
}

CustomEffectPackage BuildRetroVhsEffect() {
	return BuildNamedPreset("RetroVHS");
}

CustomEffectPackage BuildSuperReverbEffect() {
	return BuildNamedPreset("SuperReverb");
}

CustomEffectPackage BuildRoomReverbEffect() {
	return BuildNamedPreset("RoomReverb");
}

CustomEffectPackage BuildStudioReverbEffect() {
	return BuildNamedPreset("StudioReverb");
}

CustomEffectPackage BuildEqReverbEffect() {
	return BuildNamedPreset("EQReverb");
}

CustomEffectPackage BuildDelayReverbEffect() {
	return BuildNamedPreset("DelayReverb");
}

CustomEffectPackage BuildDelayEffect() {
	return BuildNamedPreset("Delay");
}

CustomEffectPackage BuildAirVoiceEffect() {
	return BuildNamedPreset("AIrVoice");
}

}  // namespace Engine::Audio::FX::Customs