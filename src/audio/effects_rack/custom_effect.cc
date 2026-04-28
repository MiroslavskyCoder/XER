#include "custom_effect.h"

#include "custom_effect_algorithms.h"
#include "custom_effect_eq_lite.h"
#include "custom_effect_invoke.h"
#include "custom_effect_list.h"

namespace Engine::Audio::FX {

CustomEffectPackage BuildSmokeCustomEffectPackage() {
	return BuildExampleCustomEffectPackage();
}

CustomEffectPackage BuildExampleCustomEffectPackage(const std::string& clap_plugin_reference) {
	CustomEffectPackage package;
	package.label = "custom_effect_example";
	package.render_config.block_size = 1024u;
	package.render_config.enable_multicore_render = true;

	CustomEffectNode compressor = CreateDefaultCustomEffectNode("compressor");
	compressor.label = "input_glue";
	compressor.stage_index = 0u;

	CustomEffectNode builtin_chorus = CreateDefaultCustomEffectNode("chorus");
	builtin_chorus.label = "chorus_builtin";
	builtin_chorus.stage_index = 1u;
	builtin_chorus.mix_curve = {CustomEffectCurvePoint{0u, 0.40f}};

	CustomEffectNode clap_gain = CreateDefaultCustomEffectNode("clap_plugin");
	clap_gain.label = "clap_gain_parallel";
	clap_gain.stage_index = 1u;
	clap_gain.plugin_reference = clap_plugin_reference.empty() ? std::string("builtin://gain") : clap_plugin_reference;

	CustomEffectNode eq = CreateCustomEffectLiteEqNode(4.0f);
	eq.label = "eq_finish";
	eq.stage_index = 2u;

	CustomEffectNode limiter = CreateDefaultCustomEffectNode("limiter");
	limiter.label = "limiter_out";
	limiter.stage_index = 3u;

	package.nodes = {compressor, builtin_chorus, clap_gain, eq, limiter};
	InvokeCustomEffectParameter(&package, "input_glue", "threshold_db", -15.0f);
	InvokeCustomEffectParameter(&package, "input_glue", "ratio", 3.5f);
	InvokeCustomEffectParameter(&package, "chorus_builtin", "rate_hz", 0.9f);
	InvokeCustomEffectParameter(&package, "chorus_builtin", "depth_samples", 12.0f);
	InvokeCustomEffectParameter(&package, "clap_gain_parallel", "gain_db", 2.5f);
	InvokeCustomEffectParameter(&package, "eq_finish", "gain_db", 4.0f);
	InvokeCustomEffectParameter(&package, "limiter_out", "ceiling_db", -1.2f);
	InvokeCustomEffectParameter(&package, "limiter_out", "release", 0.003f);
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

}  // namespace Engine::Audio::FX
