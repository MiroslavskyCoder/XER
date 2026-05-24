#include "fx_customs_presets.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>

#include "audio/audio_core/audio_interleave_processor.h"
#include "audio/effects_rack/custom_effect_algorithms.h"
#include "audio/effects_rack/custom_effect_core.h"
#include "audio/effects_rack/custom_effect_fxdata.h"
#include "audio/effects_rack/custom_effect_invoke.h"
#include "audio/effects_rack/fx_reverb_algorithmic.h"

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

struct SharedSpatialBusSettings {
	float crossfeed = 0.0f;
	float side_width = 1.0f;
	float center_gain = 1.0f;
	float shared_reverb_mix = 0.0f;
	float shared_reverb_room = 0.6f;
	float shared_reverb_damping = 0.3f;
	float surround_send = 0.0f;
};

bool ResolveSharedSpatialBusSettings(const std::string& normalized_name, SharedSpatialBusSettings* settings_out) {
	if (settings_out == nullptr) {
		return false;
	}
	if (normalized_name == "stereoconverter") {
		*settings_out = SharedSpatialBusSettings{0.12f, 1.35f, 0.98f, 0.0f, 0.5f, 0.2f, 0.0f};
		return true;
	}
	if (normalized_name == "superreverb") {
		*settings_out = SharedSpatialBusSettings{0.04f, 1.12f, 0.96f, 0.32f, 0.92f, 0.44f, 0.22f};
		return true;
	}
	if (normalized_name == "roomreverb") {
		*settings_out = SharedSpatialBusSettings{0.04f, 1.02f, 1.0f, 0.18f, 0.54f, 0.36f, 0.12f};
		return true;
	}
	if (normalized_name == "8dreverbstereo") {
		*settings_out = SharedSpatialBusSettings{0.16f, 1.58f, 0.94f, 0.36f, 0.76f, 0.42f, 0.32f};
		return true;
	}
	if (normalized_name == "studioreverb") {
		*settings_out = SharedSpatialBusSettings{0.03f, 1.05f, 0.98f, 0.18f, 0.62f, 0.28f, 0.12f};
		return true;
	}
	if (normalized_name == "eqreverb") {
		*settings_out = SharedSpatialBusSettings{0.05f, 1.10f, 0.98f, 0.20f, 0.58f, 0.27f, 0.12f};
		return true;
	}
	if (normalized_name == "delayreverb") {
		*settings_out = SharedSpatialBusSettings{0.08f, 1.20f, 0.97f, 0.24f, 0.74f, 0.34f, 0.16f};
		return true;
	}
	if (normalized_name == "delay") {
		*settings_out = SharedSpatialBusSettings{0.10f, 1.24f, 0.98f, 0.08f, 0.48f, 0.22f, 0.08f};
		return true;
	}
	if (normalized_name == "airvoice") {
		*settings_out = SharedSpatialBusSettings{0.05f, 1.08f, 1.0f, 0.15f, 0.54f, 0.24f, 0.10f};
		return true;
	}
	return false;
}

bool IsSpatialPreset(const std::string& normalized_name) {
	return normalized_name == "stereoconverter"
		|| normalized_name == "superreverb"
		|| normalized_name == "roomreverb"
		|| normalized_name == "8dreverbstereo"
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

	if (normalized_name == "8dreverbstereo") {
		InvokeCustomEffectParameter(package, "8d_orbit_width", "rate_hz", 0.36f + 0.08f * static_cast<float>(channel_index + 1));
		InvokeCustomEffectParameter(package, "8d_orbit_width", "depth_samples", 24.0f + 8.0f * pan_abs);
		InvokeCustomEffectParameter(package, "8d_orbit_micro_delay", "delay_samples", 74.0f + 38.0f * static_cast<float>(channel_index + 1));
		InvokeCustomEffectParameter(package, "8d_orbit_micro_delay", "feedback", 0.14f + 0.08f * pan_abs);
		InvokeCustomEffectParameter(package, "8d_orbit_room", "room_size", std::clamp(0.64f + 0.18f * (1.0f - pan_abs), 0.45f, 0.92f));
		InvokeCustomEffectParameter(package, "8d_orbit_room", "damping", std::clamp(0.34f + 0.14f * pan_abs, 0.18f, 0.68f));
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

bool ApplyEightChannelOrbitBus(
	float sample_rate,
	std::vector<std::vector<float>>* channels,
	CustomEffectReport* aggregate_report,
	std::string* error_out) {
	if (channels == nullptr || channels->size() < 2u) {
		return true;
	}
	const size_t frame_count = channels->front().size();
	if (frame_count == 0u || sample_rate <= 0.0f) {
		return true;
	}
	for (const auto& channel : *channels) {
		if (channel.size() != frame_count) {
			if (error_out != nullptr) {
				*error_out = "8D orbit bus channels are not aligned";
			}
			return false;
		}
	}

	constexpr size_t kVirtualChannels = 8u;
	constexpr float kPi = 3.14159265358979323846f;
	constexpr float kOrbitRateHz = 0.18f;
	std::array<std::vector<float>, kVirtualChannels> virtual_inputs;
	std::array<std::vector<float>, kVirtualChannels> virtual_outputs;
	for (size_t channel = 0; channel < kVirtualChannels; ++channel) {
		virtual_inputs[channel].assign(frame_count, 0.0f);
		virtual_outputs[channel].assign(frame_count, 0.0f);
	}

	for (size_t frame = 0; frame < frame_count; ++frame) {
		float source = 0.0f;
		for (const auto& channel : *channels) {
			source += channel[frame];
		}
		source /= static_cast<float>(channels->size());
		const float time_seconds = static_cast<float>(frame) / sample_rate;
		const float orbit = time_seconds * kOrbitRateHz * 2.0f * kPi;
		for (size_t virtual_channel = 0; virtual_channel < kVirtualChannels; ++virtual_channel) {
			const float channel_angle = 2.0f * kPi * static_cast<float>(virtual_channel) / static_cast<float>(kVirtualChannels);
			const float distance = 0.5f + 0.5f * std::cos(orbit - channel_angle);
			const float lobe = 0.18f + std::pow(std::max(0.0f, distance), 2.2f) * 0.82f;
			const float shimmer = 0.85f + 0.15f * std::sin(orbit * 0.5f + channel_angle * 1.7f);
			virtual_inputs[virtual_channel][frame] = source * lobe * shimmer;
		}
	}

	for (size_t virtual_channel = 0; virtual_channel < kVirtualChannels; ++virtual_channel) {
		ReverbAlgorithmic room;
		if (!room.Initialize(sample_rate, std::max<size_t>(8192u, static_cast<size_t>(sample_rate * 1.4f)))) {
			if (error_out != nullptr) {
				*error_out = "failed to initialize 8D virtual room channel";
			}
			return false;
		}
		const float phase = static_cast<float>(virtual_channel) / static_cast<float>(kVirtualChannels - 1u);
		room.SetRoomSize(0.62f + 0.22f * phase);
		room.SetDamping(0.34f + 0.16f * (1.0f - phase));
		room.SetMix(1.0f);
		if (!room.ProcessBlock(virtual_inputs[virtual_channel].data(), frame_count, virtual_outputs[virtual_channel].data())) {
			if (error_out != nullptr) {
				*error_out = "8D virtual room processing failed";
			}
			return false;
		}
		if (aggregate_report != nullptr) {
			aggregate_report->node_reports.push_back("8d_virtual_channel=" + std::to_string(virtual_channel) + ":" + room.GetReport());
		}
	}

	for (size_t frame = 0; frame < frame_count; ++frame) {
		const float dry_left = (*channels)[0][frame];
		const float dry_right = (*channels)[1][frame];
		float wet_left = 0.0f;
		float wet_right = 0.0f;
		const float time_seconds = static_cast<float>(frame) / sample_rate;
		const float orbit = time_seconds * kOrbitRateHz * 2.0f * kPi;
		for (size_t virtual_channel = 0; virtual_channel < kVirtualChannels; ++virtual_channel) {
			const float angle = orbit + 2.0f * kPi * static_cast<float>(virtual_channel) / static_cast<float>(kVirtualChannels);
			const float pan = std::sin(angle);
			const float left_gain = std::sqrt(std::max(0.0f, (1.0f - pan) * 0.5f));
			const float right_gain = std::sqrt(std::max(0.0f, (1.0f + pan) * 0.5f));
			const float depth = 0.74f + 0.26f * std::cos(angle);
			const float wet = virtual_outputs[virtual_channel][frame] * depth;
			wet_left += wet * left_gain;
			wet_right += wet * right_gain;
		}
		wet_left /= static_cast<float>(kVirtualChannels) * 0.62f;
		wet_right /= static_cast<float>(kVirtualChannels) * 0.62f;
		(*channels)[0][frame] = dry_left * 0.72f + wet_left * 0.42f;
		(*channels)[1][frame] = dry_right * 0.72f + wet_right * 0.42f;
		for (size_t channel_index = 2; channel_index < channels->size(); ++channel_index) {
			const float blend = (wet_left + wet_right) * 0.5f;
			(*channels)[channel_index][frame] = (*channels)[channel_index][frame] * 0.65f + blend * 0.35f;
		}
	}

	if (aggregate_report != nullptr) {
		aggregate_report->stage_reports.push_back("8d_orbit_bus=virtual_channels=8,orbit_rate_hz=" + std::to_string(kOrbitRateHz) + ",downmix=stereo");
	}
	return true;
}

bool ApplySharedSpatialBus(
	const std::string& normalized_name,
	float sample_rate,
	std::vector<std::vector<float>>* channels,
	CustomEffectReport* aggregate_report,
	std::string* error_out) {
	if (channels == nullptr || channels->size() < 2u) {
		return true;
	}
	if (normalized_name == "8dreverbstereo") {
		return ApplyEightChannelOrbitBus(sample_rate, channels, aggregate_report, error_out);
	}
	SharedSpatialBusSettings settings;
	if (!ResolveSharedSpatialBusSettings(normalized_name, &settings)) {
		return true;
	}
	const size_t frame_count = channels->front().size();
	for (const auto& channel : *channels) {
		if (channel.size() != frame_count) {
			if (error_out != nullptr) {
				*error_out = "shared spatial bus channels are not aligned";
			}
			return false;
		}
	}

	std::vector<float> mid(frame_count, 0.0f);
	std::vector<float> side(frame_count, 0.0f);
	std::vector<float> ambience_input(frame_count, 0.0f);
	std::vector<float> ambience_output(frame_count, 0.0f);
	for (size_t frame = 0; frame < frame_count; ++frame) {
		const float left = (*channels)[0][frame];
		const float right = (*channels)[1][frame];
		const float cross_left = left * (1.0f - settings.crossfeed) + right * settings.crossfeed;
		const float cross_right = right * (1.0f - settings.crossfeed) + left * settings.crossfeed;
		mid[frame] = 0.5f * (cross_left + cross_right);
		side[frame] = 0.5f * (cross_left - cross_right) * settings.side_width;
		float shared_bus = 0.0f;
		for (const auto& channel : *channels) {
			shared_bus += channel[frame];
		}
		shared_bus /= static_cast<float>(channels->size());
		ambience_input[frame] = 0.40f * shared_bus + 0.60f * side[frame];
	}

	if (settings.shared_reverb_mix > 0.0f) {
		ReverbAlgorithmic shared_reverb;
		if (!shared_reverb.Initialize(sample_rate, std::max<size_t>(4096u, frame_count / 2u + 2048u))) {
			if (error_out != nullptr) {
				*error_out = "failed to initialize shared spatial reverb";
			}
			return false;
		}
		shared_reverb.SetRoomSize(settings.shared_reverb_room);
		shared_reverb.SetDamping(settings.shared_reverb_damping);
		shared_reverb.SetMix(1.0f);
		if (!shared_reverb.ProcessBlock(ambience_input.data(), frame_count, ambience_output.data())) {
			if (error_out != nullptr) {
				*error_out = "shared spatial reverb processing failed";
			}
			return false;
		}
		if (aggregate_report != nullptr) {
			aggregate_report->stage_reports.push_back(
				"shared_spatial_bus=crossfeed+ms+reverb,crossfeed=" + std::to_string(settings.crossfeed) +
				",width=" + std::to_string(settings.side_width) +
				",reverb_mix=" + std::to_string(settings.shared_reverb_mix));
			aggregate_report->node_reports.push_back("shared_spatial_reverb:" + shared_reverb.GetReport());
		}
	} else if (aggregate_report != nullptr) {
		aggregate_report->stage_reports.push_back(
			"shared_spatial_bus=crossfeed+ms,crossfeed=" + std::to_string(settings.crossfeed) +
			",width=" + std::to_string(settings.side_width));
	}

	for (size_t frame = 0; frame < frame_count; ++frame) {
		const float wet = ambience_output[frame] * settings.shared_reverb_mix;
		(*channels)[0][frame] = mid[frame] * settings.center_gain + side[frame] + wet * 0.45f;
		(*channels)[1][frame] = mid[frame] * settings.center_gain - side[frame] + wet * 0.45f;
		for (size_t channel_index = 2; channel_index < channels->size(); ++channel_index) {
			const float position = ChannelPan(static_cast<int>(channel_index), static_cast<int>(channels->size()));
			const float spread_weight = 0.70f - 0.30f * std::abs(position);
			(*channels)[channel_index][frame] = (*channels)[channel_index][frame] * (1.0f - settings.surround_send * 0.35f)
				+ wet * settings.surround_send * spread_weight;
		}
	}
	return true;
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
	if (IsSpatialPreset(normalized) && !ApplySharedSpatialBus(normalized, sample_rate, &output_channels, &aggregate_report, error_out)) {
		return false;
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

CustomEffectPackage Build8DReverbStereoEffect() {
	return BuildNamedPreset("8DReverbStereo");
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