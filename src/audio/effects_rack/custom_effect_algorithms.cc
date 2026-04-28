#include "custom_effect_algorithms.h"

#include "custom_effect_eq_lite.h"

namespace Engine::Audio::FX {

std::vector<std::string> ListCustomEffectAlgorithms() {
	return {"passthrough", "compressor", "chorus", "parametric_eq", "limiter", "clap_plugin"};
}

CustomEffectNode CreateDefaultCustomEffectNode(const std::string& algorithm_name) {
	CustomEffectAlgorithm algorithm = CustomEffectAlgorithm::kUnknown;
	ParseCustomEffectAlgorithm(algorithm_name, &algorithm);
	CustomEffectNode node;
	node.label = algorithm_name;
	node.backend = CustomEffectBackend::kBuiltinAlgorithm;
	node.algorithm = algorithm;
	switch (algorithm) {
	case CustomEffectAlgorithm::kPassthrough:
		node.mix_curve.push_back(CustomEffectCurvePoint{0u, 1.0f});
		break;
	case CustomEffectAlgorithm::kCompressor:
		node.mix_curve.push_back(CustomEffectCurvePoint{0u, 0.85f});
		node.parameters.push_back(CustomEffectParameter{"threshold_db", -16.0f});
		node.parameters.push_back(CustomEffectParameter{"ratio", 3.0f});
		break;
	case CustomEffectAlgorithm::kChorus:
		node.mix_curve.push_back(CustomEffectCurvePoint{0u, 0.35f});
		node.parameters.push_back(CustomEffectParameter{"rate_hz", 0.8f});
		node.parameters.push_back(CustomEffectParameter{"depth_samples", 14.0f});
		break;
	case CustomEffectAlgorithm::kParametricEq:
		return CreateCustomEffectLiteEqNode(4.5f);
	case CustomEffectAlgorithm::kLimiter:
		node.mix_curve.push_back(CustomEffectCurvePoint{0u, 1.0f});
		node.parameters.push_back(CustomEffectParameter{"ceiling_db", -1.0f});
		node.parameters.push_back(CustomEffectParameter{"release", 0.002f});
		break;
	case CustomEffectAlgorithm::kPlugin:
		node.label = "clap_plugin";
		node.backend = CustomEffectBackend::kClapPlugin;
		node.plugin_reference = "builtin://gain";
		node.parameters.push_back(CustomEffectParameter{"gain_db", 3.0f});
		node.mix_curve.push_back(CustomEffectCurvePoint{0u, 1.0f});
		break;
	case CustomEffectAlgorithm::kUnknown:
		break;
	}
	return node;
}

}  // namespace Engine::Audio::FX
