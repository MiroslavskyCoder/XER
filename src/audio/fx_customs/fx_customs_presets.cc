#include "fx_customs_presets.h"

#include <algorithm>
#include <cctype>

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