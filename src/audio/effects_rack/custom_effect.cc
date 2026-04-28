#include "custom_effect.h"

#include "custom_effect_algorithms.h"
#include "custom_effect_core.h"
#include "custom_effect_eq_lite.h"
#include "custom_effect_fxdata.h"
#include "custom_effect_invoke.h"
#include "custom_effect_list.h"

#include "audio/audio_core/audio_interleave_processor.h"

#include <algorithm>
#include <cctype>

namespace Engine::Audio::FX {

namespace {

std::string NormalizeEffectName(std::string name) {
	std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
	return name;
}

CustomEffectNode MakeNode(const std::string& algorithm_name, const std::string& label, uint32_t stage_index, float mix = 1.0f) {
	CustomEffectNode node = CreateDefaultCustomEffectNode(algorithm_name);
	node.label = label;
	node.stage_index = stage_index;
	node.mix_curve = {CustomEffectCurvePoint{0u, mix}};
	return node;
}

void SetParam(CustomEffectNode* node, const std::string& name, float value) {
	SetCustomEffectParameterValue(&node->parameters, name, value);
}

CustomEffectPackage MakePresetPackage(const std::string& label, std::vector<CustomEffectNode> nodes) {
	CustomEffectPackage package;
	package.label = label;
	package.render_config.block_size = 1024u;
	package.render_config.enable_multicore_render = true;
	package.nodes = std::move(nodes);
	return package;
}

}  // namespace

CustomEffectPackage BuildSmokeCustomEffectPackage() {
	return BuildExampleCustomEffectPackage();
}

std::vector<std::string> ListAvailableCustomEffects() {
	return ListCustomEffectPresets();
}

CustomEffectPackage BuildNamedCustomEffectPackage(const std::string& effect_name, const std::string& clap_plugin_reference) {
	const std::string normalized = NormalizeEffectName(effect_name);

	if (normalized == "bassboost") {
		CustomEffectNode eq = MakeNode("parametric_eq", "bass_eq", 0u, 0.95f);
		SetParam(&eq, "frequency_hz", 110.0f);
		SetParam(&eq, "q", 0.7f);
		SetParam(&eq, "gain_db", 8.0f);
		CustomEffectNode limiter = MakeNode("limiter", "bass_limiter", 1u, 1.0f);
		SetParam(&limiter, "ceiling_db", -0.8f);
		return MakePresetPackage("BassBoost", {eq, limiter});
	}
	if (normalized == "eq") {
		CustomEffectNode low = MakeNode("parametric_eq", "eq_body", 0u, 1.0f);
		SetParam(&low, "frequency_hz", 220.0f);
		SetParam(&low, "q", 0.9f);
		SetParam(&low, "gain_db", 3.0f);
		CustomEffectNode high = MakeNode("parametric_eq", "eq_presence", 1u, 1.0f);
		SetParam(&high, "frequency_hz", 4200.0f);
		SetParam(&high, "q", 0.7f);
		SetParam(&high, "gain_db", 2.5f);
		return MakePresetPackage("EQ", {low, high});
	}
	if (normalized == "high") {
		CustomEffectNode high = MakeNode("parametric_eq", "high_boost", 0u, 1.0f);
		SetParam(&high, "frequency_hz", 8200.0f);
		SetParam(&high, "q", 0.8f);
		SetParam(&high, "gain_db", 4.5f);
		return MakePresetPackage("High", {high});
	}
	if (normalized == "airhigh") {
		CustomEffectNode air = MakeNode("parametric_eq", "air_high", 0u, 1.0f);
		SetParam(&air, "frequency_hz", 12000.0f);
		SetParam(&air, "q", 0.6f);
		SetParam(&air, "gain_db", 6.5f);
		return MakePresetPackage("AIRHigh", {air});
	}
	if (normalized == "curveair") {
		CustomEffectNode sheen = MakeNode("parametric_eq", "curve_air_sheen", 0u, 1.0f);
		SetParam(&sheen, "frequency_hz", 6500.0f);
		SetParam(&sheen, "q", 0.7f);
		SetParam(&sheen, "gain_db", 2.5f);
		CustomEffectNode air = MakeNode("parametric_eq", "curve_air_top", 1u, 1.0f);
		SetParam(&air, "frequency_hz", 14500.0f);
		SetParam(&air, "q", 0.5f);
		SetParam(&air, "gain_db", 6.0f);
		return MakePresetPackage("CurveAIR", {sheen, air});
	}
	if (normalized == "deesser") {
		CustomEffectNode cut = MakeNode("parametric_eq", "deesser_cut", 0u, 1.0f);
		SetParam(&cut, "frequency_hz", 6800.0f);
		SetParam(&cut, "q", 3.0f);
		SetParam(&cut, "gain_db", -6.0f);
		CustomEffectNode comp = MakeNode("compressor", "deesser_control", 1u, 1.0f);
		SetParam(&comp, "threshold_db", -28.0f);
		SetParam(&comp, "ratio", 7.0f);
		return MakePresetPackage("DeEsser", {cut, comp});
	}
	if (normalized == "compressor") {
		CustomEffectNode comp = MakeNode("compressor", "compressor_main", 0u, 1.0f);
		SetParam(&comp, "threshold_db", -18.0f);
		SetParam(&comp, "ratio", 4.0f);
		return MakePresetPackage("Compressor", {comp});
	}
	if (normalized == "limiter") {
		CustomEffectNode limiter = MakeNode("limiter", "limiter_main", 0u, 1.0f);
		SetParam(&limiter, "ceiling_db", -0.5f);
		SetParam(&limiter, "release", 0.002f);
		return MakePresetPackage("Limiter", {limiter});
	}
	if (normalized == "autotune") {
		CustomEffectNode pitch = MakeNode("pitch_shift", "autotune_pitch", 0u, 1.0f);
		SetParam(&pitch, "pitch_ratio", 1.059463f);
		CustomEffectNode limiter = MakeNode("limiter", "autotune_limiter", 1u, 1.0f);
		return MakePresetPackage("AutoTune", {pitch, limiter});
	}
	if (normalized == "pitchkey") {
		CustomEffectNode pitch = MakeNode("pitch_shift", "pitchkey_shift", 0u, 1.0f);
		SetParam(&pitch, "pitch_ratio", 1.122462f);
		return MakePresetPackage("PitchKey", {pitch});
	}
	if (normalized == "autopitch") {
		CustomEffectNode pitch = MakeNode("pitch_shift", "autopitch_shift", 0u, 1.0f);
		SetParam(&pitch, "pitch_ratio", 0.943874f);
		CustomEffectNode tube = MakeNode("tube", "autopitch_tone", 1u, 0.35f);
		return MakePresetPackage("AutoPitch", {pitch, tube});
	}
	if (normalized == "stereoconverter") {
		CustomEffectNode chorus = MakeNode("chorus", "stereo_width_chorus", 0u, 0.30f);
		SetParam(&chorus, "rate_hz", 0.7f);
		SetParam(&chorus, "depth_samples", 22.0f);
		CustomEffectNode delay = MakeNode("delay", "stereo_width_delay", 1u, 0.20f);
		SetParam(&delay, "delay_samples", 96.0f);
		SetParam(&delay, "feedback", 0.12f);
		return MakePresetPackage("Stereo Converter", {chorus, delay});
	}
	if (normalized == "vhs") {
		CustomEffectNode tube = MakeNode("tube", "vhs_tube", 0u, 0.65f);
		SetParam(&tube, "drive", 2.8f);
		CustomEffectNode crush = MakeNode("bitcrush", "vhs_crush", 1u, 0.55f);
		SetParam(&crush, "bit_depth", 9.0f);
		SetParam(&crush, "downsample_factor", 3.0f);
		CustomEffectNode delay = MakeNode("delay", "vhs_delay", 2u, 0.25f);
		SetParam(&delay, "delay_samples", 720.0f);
		SetParam(&delay, "feedback", 0.18f);
		return MakePresetPackage("VHS", {tube, crush, delay});
	}
	if (normalized == "retrovhs") {
		CustomEffectNode crush = MakeNode("bitcrush", "retro_vhs_crush", 0u, 0.65f);
		SetParam(&crush, "bit_depth", 8.0f);
		SetParam(&crush, "downsample_factor", 4.0f);
		CustomEffectNode flanger = MakeNode("flanger", "retro_vhs_flanger", 1u, 0.30f);
		SetParam(&flanger, "rate_hz", 0.18f);
		SetParam(&flanger, "depth_samples", 14.0f);
		SetParam(&flanger, "feedback", 0.28f);
		CustomEffectNode reverb = MakeNode("reverb_algorithmic", "retro_vhs_room", 2u, 0.25f);
		SetParam(&reverb, "room_size", 0.55f);
		SetParam(&reverb, "damping", 0.35f);
		return MakePresetPackage("RetroVHS", {crush, flanger, reverb});
	}
	if (normalized == "superreverb") {
		CustomEffectNode reverb = MakeNode("reverb_algorithmic", "super_reverb", 0u, 0.65f);
		SetParam(&reverb, "room_size", 0.95f);
		SetParam(&reverb, "damping", 0.45f);
		return MakePresetPackage("SuperReverb", {reverb});
	}
	if (normalized == "roomreverb") {
		CustomEffectNode reverb = MakeNode("reverb_algorithmic", "room_reverb", 0u, 0.25f);
		SetParam(&reverb, "room_size", 0.35f);
		SetParam(&reverb, "damping", 0.22f);
		return MakePresetPackage("RoomReverb", {reverb});
	}
	if (normalized == "studioreverb") {
		CustomEffectNode reverb = MakeNode("reverb_algorithmic", "studio_reverb", 0u, 0.35f);
		SetParam(&reverb, "room_size", 0.60f);
		SetParam(&reverb, "damping", 0.28f);
		return MakePresetPackage("StudioReverb", {reverb});
	}
	if (normalized == "eqreverb") {
		CustomEffectNode eq = MakeNode("parametric_eq", "eq_reverb_eq", 0u, 1.0f);
		SetParam(&eq, "frequency_hz", 4200.0f);
		SetParam(&eq, "q", 0.7f);
		SetParam(&eq, "gain_db", 3.0f);
		CustomEffectNode reverb = MakeNode("reverb_algorithmic", "eq_reverb_room", 1u, 0.30f);
		SetParam(&reverb, "room_size", 0.58f);
		SetParam(&reverb, "damping", 0.27f);
		return MakePresetPackage("EQReverb", {eq, reverb});
	}
	if (normalized == "delayreverb") {
		CustomEffectNode delay = MakeNode("delay", "delay_reverb_delay", 0u, 0.32f);
		SetParam(&delay, "delay_samples", 880.0f);
		SetParam(&delay, "feedback", 0.28f);
		CustomEffectNode reverb = MakeNode("reverb_algorithmic", "delay_reverb_room", 1u, 0.35f);
		SetParam(&reverb, "room_size", 0.72f);
		SetParam(&reverb, "damping", 0.34f);
		return MakePresetPackage("DelayReverb", {delay, reverb});
	}
	if (normalized == "delay") {
		CustomEffectNode delay = MakeNode("delay", "delay_main", 0u, 0.45f);
		SetParam(&delay, "delay_samples", 960.0f);
		SetParam(&delay, "feedback", 0.35f);
		return MakePresetPackage("Delay", {delay});
	}
	if (normalized == "airvoice") {
		CustomEffectNode gate = MakeNode("gate", "airvoice_gate", 0u, 1.0f);
		SetParam(&gate, "threshold_db", -48.0f);
		SetParam(&gate, "hold_samples", 384.0f);
		CustomEffectNode deesser = MakeNode("parametric_eq", "airvoice_deesser", 1u, 1.0f);
		SetParam(&deesser, "frequency_hz", 6500.0f);
		SetParam(&deesser, "q", 2.4f);
		SetParam(&deesser, "gain_db", -4.5f);
		CustomEffectNode comp = MakeNode("compressor", "airvoice_comp", 2u, 1.0f);
		SetParam(&comp, "threshold_db", -20.0f);
		SetParam(&comp, "ratio", 4.5f);
		CustomEffectNode air = MakeNode("parametric_eq", "airvoice_air", 3u, 1.0f);
		SetParam(&air, "frequency_hz", 11800.0f);
		SetParam(&air, "q", 0.55f);
		SetParam(&air, "gain_db", 5.5f);
		CustomEffectNode limiter = MakeNode("limiter", "airvoice_limiter", 4u, 1.0f);
		SetParam(&limiter, "ceiling_db", -1.0f);
		return MakePresetPackage("AIrVoice", {gate, deesser, comp, air, limiter});
	}

	return BuildExampleCustomEffectPackage(clap_plugin_reference);
}

CustomEffectPackage BuildExampleCustomEffectPackage(const std::string& clap_plugin_reference) {
	CustomEffectPackage package = BuildNamedCustomEffectPackage("AIrVoice", clap_plugin_reference);
	package.label = "custom_effect_example";
	CustomEffectNode clap_gain = MakeNode("clap_plugin", "clap_gain_parallel", 2u, 1.0f);
	clap_gain.plugin_reference = clap_plugin_reference.empty() ? std::string("builtin://gain") : clap_plugin_reference;
	SetParam(&clap_gain, "gain_db", 2.5f);
	package.nodes.push_back(clap_gain);
	return package;
}

bool RenderCustomEffectExample(
	float sample_rate,
	const std::vector<float>& input,
	std::vector<float>* output,
	CustomEffectReport* report_out,
	std::string* error_out) {
	return RunCustomEffectPackage(BuildExampleCustomEffectPackage(), sample_rate, input, output, report_out, error_out);
}

bool RunCustomEffectPackage(
	const CustomEffectPackage& package,
	float sample_rate,
	const std::vector<float>& input,
	std::vector<float>* output,
	CustomEffectReport* report_out,
	std::string* error_out) {
	CustomEffectFlow flow;
	if (!flow.Initialize(package, sample_rate, package.render_config.block_size, error_out)) {
		return false;
	}
	if (!flow.Process(input, output, error_out)) {
		return false;
	}
	if (report_out != nullptr) {
		*report_out = flow.GetLastReport();
	}
	return true;
}

bool RunCustomEffectPackageInterleaved(
	const CustomEffectPackage& package,
	float sample_rate,
	const std::vector<float>& input,
	int channels,
	std::vector<float>* output,
	CustomEffectReport* report_out,
	std::string* error_out) {
	if (output == nullptr || channels <= 0 || input.empty() || (input.size() % static_cast<size_t>(channels)) != 0u) {
		if (error_out != nullptr) {
			*error_out = "custom effect interleaved buffer is invalid";
		}
		return false;
	}

	const size_t frame_count = input.size() / static_cast<size_t>(channels);
	std::vector<std::vector<float>> input_channels;
	if (!Engine::Audio::Core::AudioInterleaveProcessor::DeinterleavePlanar(
			input.data(),
			channels,
			frame_count,
			input_channels)) {
		if (error_out != nullptr) {
			*error_out = "failed to deinterleave custom effect input";
		}
		return false;
	}

	std::vector<std::vector<float>> output_channels(static_cast<size_t>(channels));
	CustomEffectReport aggregate_report;
	aggregate_report.label = package.label;
	aggregate_report.stage_count = 0u;
	aggregate_report.node_count = 0u;
	aggregate_report.channel_count = static_cast<size_t>(channels);
	aggregate_report.worker_count_used = 1u;
	for (int channel_index = 0; channel_index < channels; ++channel_index) {
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
			*error_out = "failed to interleave custom effect output";
		}
		return false;
	}
	ComputeCustomEffectReportStats(*output, &aggregate_report);
	if (report_out != nullptr) {
		*report_out = aggregate_report;
	}
	return true;
}

}  // namespace Engine::Audio::FX
