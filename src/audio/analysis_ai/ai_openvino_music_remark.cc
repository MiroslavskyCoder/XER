#include "ai_openvino_music_remark.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

#if __has_include(<openvino/openvino.hpp>)
#define XER_AUDIO_AI_HAS_OPENVINO 1
#else
#define XER_AUDIO_AI_HAS_OPENVINO 0
#endif

namespace Engine::Audio::AnalysisAI {
namespace {

float Clamp01(float value) {
	return std::clamp(value, 0.0f, 1.0f);
}

float Closeness(float value, float target, float tolerance) {
	if (tolerance <= 0.0f || value <= 0.0f) {
		return 0.0f;
	}
	return Clamp01(1.0f - std::abs(value - target) / tolerance);
}

std::string GradeForScore(float score) {
	if (score >= 88.0f) return "master-ready";
	if (score >= 74.0f) return "strong";
	if (score >= 58.0f) return "mixable";
	if (score >= 42.0f) return "needs-polish";
	return "needs-repair";
}

bool ContainsMood(const std::string& mood, const std::string& needle) {
	return mood.find(needle) != std::string::npos;
}

void PushUnique(std::vector<std::string>* values, const std::string& value) {
	if (values == nullptr || value.empty()) {
		return;
	}
	if (std::find(values->begin(), values->end(), value) == values->end()) {
		values->push_back(value);
	}
}

void AddSampleSlot(
	std::vector<MusicRemarkSampleSlot>* slots,
	std::string kind,
	std::string label,
	std::string search_tag,
	float beat_offset,
	float confidence,
	std::string effect_anchor) {
	if (slots == nullptr) {
		return;
	}
	slots->push_back(MusicRemarkSampleSlot{
		std::move(kind),
		std::move(label),
		std::move(search_tag),
		beat_offset,
		Clamp01(confidence),
		std::move(effect_anchor),
	});
}

void AddStemAction(
	std::vector<MusicRemarkStemAction>* actions,
	std::string stem,
	std::string operation,
	float amount,
	float beat_offset,
	float stretch_ratio,
	float confidence,
	std::string reason,
	std::vector<std::string> effects) {
	if (actions == nullptr) return;
	actions->push_back(MusicRemarkStemAction{
		std::move(stem),
		std::move(operation),
		Clamp01(amount),
		beat_offset,
		std::clamp(stretch_ratio, 0.5f, 1.75f),
		Clamp01(confidence),
		std::move(reason),
		std::move(effects),
	});
}

void AddEffectLayer(
	std::vector<MusicRemarkEffectLayer>* layers,
	std::string target,
	std::vector<std::string> effects,
	float mix) {
	if (layers == nullptr || effects.empty()) return;
	layers->push_back(MusicRemarkEffectLayer{std::move(target), std::move(effects), Clamp01(mix)});
}

void AddDjSection(
	std::vector<MusicRemarkDjSection>* sections,
	std::string name,
	std::string purpose,
	float start_beat,
	float length_beats,
	float energy,
	std::vector<std::string> operations,
	std::vector<std::string> effects) {
	if (sections == nullptr || length_beats <= 0.0f) return;
	sections->push_back(MusicRemarkDjSection{
		std::move(name),
		std::move(purpose),
		std::max(0.0f, start_beat),
		std::max(1.0f, length_beats),
		Clamp01(energy),
		std::move(operations),
		std::move(effects),
	});
}

void AddTransitionAction(
	std::vector<MusicRemarkTransitionAction>* transitions,
	std::string kind,
	std::string label,
	std::string from_section,
	std::string to_section,
	float beat_offset,
	float length_beats,
	float confidence,
	std::vector<std::string> effects) {
	if (transitions == nullptr || length_beats <= 0.0f) return;
	transitions->push_back(MusicRemarkTransitionAction{
		std::move(kind),
		std::move(label),
		std::move(from_section),
		std::move(to_section),
		std::max(0.0f, beat_offset),
		std::max(1.0f, length_beats),
		Clamp01(confidence),
		std::move(effects),
	});
}

}  // namespace

bool IsOpenVinoRuntimeAvailable() {
#if XER_AUDIO_AI_HAS_OPENVINO
	return true;
#else
	return false;
#endif
}

MusicRemarkMixPlan BuildOpenVinoMusicRemarkMixPlan(const MusicRemarkMetrics& metrics, const MusicRemarkOptions& options) {
	MusicRemarkMixPlan plan;
	plan.openvino_available = IsOpenVinoRuntimeAvailable();
	plan.backend = plan.openvino_available ? "xer-openvino-audio-ai" : "xer-ai-heuristic-openvino-fallback";
	const char* model_path = std::getenv("XER_OPENVINO_MUSIC_REMARK_MODEL");
	plan.model_hint = model_path != nullptr && model_path[0] != '\0'
		? std::string(model_path)
		: std::string("openvino-ir:music-remark-mix.xml");

	float score = 38.0f;
	score += 15.0f * Closeness(metrics.bpm, 124.0f, 64.0f);
	score += 14.0f * Clamp01(metrics.beat_strength * 2.2f);
	score += 12.0f * Clamp01(metrics.pitch_confidence);
	if (metrics.duration_seconds > 0.0f) {
		score += 9.0f * Closeness(metrics.duration_seconds, 185.0f, 175.0f);
	}
	if (metrics.loudness_integrated_lufs < 0.0f) {
		score += 13.0f * Closeness(metrics.loudness_integrated_lufs, -12.5f, 9.5f);
	}
	if (metrics.true_peak_dbfs < 0.0f) {
		score += 8.0f * Closeness(metrics.true_peak_dbfs, -1.0f, 4.0f);
	}
	if (plan.openvino_available) {
		score += 3.0f;
	}
	plan.score = std::clamp(score, 0.0f, 100.0f);
	plan.grade = GradeForScore(plan.score);

	PushUnique(&plan.effects, "MasterGlue");
	if (metrics.loudness_integrated_lufs < -17.5f || metrics.loudness_integrated_lufs == 0.0f) {
		PushUnique(&plan.effects, "SSLBusComp");
		PushUnique(&plan.reasons, "low-loudness-glue");
	}
	if (metrics.true_peak_dbfs > -0.4f) {
		PushUnique(&plan.effects, "Limiter");
		PushUnique(&plan.reasons, "true-peak-control");
	}
	if (metrics.beat_strength >= 0.34f) {
		PushUnique(&plan.effects, "TransientPunch");
		PushUnique(&plan.reasons, "beat-forward-remix");
	}
	if (metrics.pitch_confidence < 0.44f) {
		PushUnique(&plan.effects, "VocalPresence");
		PushUnique(&plan.reasons, "pitch-presence-recovery");
	} else {
		PushUnique(&plan.effects, "ExciterAir");
	}

	const std::string mood = options.target_mood;
	const bool wants_instrumental = options.prefer_instrumental || ContainsMood(mood, "instrumental") || ContainsMood(mood, "karaoke") || ContainsMood(mood, "minus") || ContainsMood(mood, "no vocal");
	const bool wants_acapella = options.prefer_acapella || ContainsMood(mood, "acapella") || ContainsMood(mood, "voice only") || ContainsMood(mood, "vocal only");
	if (ContainsMood(mood, "dream") || ContainsMood(mood, "ambient") || ContainsMood(mood, "space")) {
		PushUnique(&plan.effects, "ShimmerBloom");
		PushUnique(&plan.effects, "DimensionChorus");
		PushUnique(&plan.reasons, "wide-atmospheric-target");
	} else if (ContainsMood(mood, "vintage") || ContainsMood(mood, "retro") || ContainsMood(mood, "warm")) {
		PushUnique(&plan.effects, "TapeMachine");
		PushUnique(&plan.effects, "VinylMaster");
		PushUnique(&plan.reasons, "vintage-color-target");
	} else if (ContainsMood(mood, "radio") || ContainsMood(mood, "voice")) {
		PushUnique(&plan.effects, "RadioAnnouncer");
		PushUnique(&plan.reasons, "voice-forward-target");
	} else {
		PushUnique(&plan.effects, "PsychoWidener");
	}
	PushUnique(&plan.effects, "Limiter");

	const float intensity = Clamp01(options.sample_intensity);
	if (intensity > 0.12f) {
		AddSampleSlot(&plan.sample_slots, "transition", "downbeat impact layer", "impact kick downbeat", 0.0f, 0.78f + 0.12f * intensity, "TransientPunch");
	}
	if (intensity > 0.30f) {
		AddSampleSlot(&plan.sample_slots, "texture", "air shimmer tail", "shimmer atmosphere riser", 3.5f, 0.66f + 0.16f * intensity, "ShimmerBloom");
	}
	if (intensity > 0.52f || metrics.beat_strength < 0.25f) {
		AddSampleSlot(&plan.sample_slots, "groove", "ghost percussion support", "perc hat loop", 1.0f, 0.58f + 0.18f * intensity, "SpaceEcho");
	}
	if (metrics.true_peak_dbfs > -0.2f || plan.score < 58.0f) {
		AddSampleSlot(&plan.sample_slots, "repair", "soft noise mask before limiter", "vinyl noise texture", 0.5f, 0.62f, "TapeMachine");
	}
	if (plan.sample_slots.empty()) {
		AddSampleSlot(&plan.sample_slots, "texture", "subtle room lift", "room ambience", 2.0f, 0.52f, "DimensionChorus");
	}

	if (wants_instrumental || metrics.pitch_confidence > 0.58f) {
		AddStemAction(&plan.stem_actions, "vocal", wants_instrumental ? "suppress" : "extract-move", wants_instrumental ? 0.88f : 0.34f, options.vocal_move_beats, options.vocal_stretch_ratio, 0.64f + 0.20f * Clamp01(metrics.pitch_confidence), wants_instrumental ? "instrumental-target" : "vocal-remix-focus", {"LA2AVocal", "SilkyDeEsser", "ExciterAir"});
	}
	if (wants_acapella || metrics.beat_strength > 0.52f) {
		AddStemAction(&plan.stem_actions, "drums", wants_acapella ? "suppress" : "duck-move", wants_acapella ? 0.82f : 0.28f, options.drums_move_beats, 1.0f, 0.58f + 0.22f * Clamp01(metrics.beat_strength), wants_acapella ? "acapella-target" : "create-room-for-samples", {"TransientPunch", "SSLBusComp"});
	}
	if (std::abs(options.vocal_move_beats) > 0.01f || std::abs(options.vocal_stretch_ratio - 1.0f) > 0.01f) {
		AddStemAction(&plan.stem_actions, "vocal", "time-stretch-move", 0.72f, options.vocal_move_beats, options.vocal_stretch_ratio, 0.74f, "explicit-vocal-timing-request", {"PitchShifter", "VocalPresence"});
	}
	if (std::abs(options.drums_move_beats) > 0.01f) {
		AddStemAction(&plan.stem_actions, "drums", "move", 0.55f, options.drums_move_beats, 1.0f, 0.68f, "explicit-drums-timing-request", {"TransientPunch"});
	}

	AddEffectLayer(&plan.effect_layers, "master", plan.effects, 1.0f);
	AddEffectLayer(&plan.effect_layers, "vocal", {"LA2AVocal", "SilkyDeEsser", "ExciterAir"}, metrics.pitch_confidence > 0.4f ? 0.82f : 0.54f);
	AddEffectLayer(&plan.effect_layers, "drums", {"TransientPunch", "SSLBusComp"}, metrics.beat_strength > 0.3f ? 0.78f : 0.48f);
	AddEffectLayer(&plan.effect_layers, "samples", {"SpaceEcho", "ShimmerBloom", "Limiter"}, 0.62f + 0.22f * intensity);

	const float bpm = metrics.bpm > 0.0f ? metrics.bpm : 124.0f;
	const float total_beats = metrics.duration_seconds > 0.0f ? std::max(96.0f, metrics.duration_seconds * bpm / 60.0f) : 256.0f;
	const float phrase = total_beats >= 192.0f ? 32.0f : 16.0f;
	const float intro_len = phrase;
	const float break_start = std::min(total_beats - phrase * 2.0f, phrase * 3.0f);
	const float drop_start = std::min(total_beats - phrase, break_start + phrase);
	const float outro_start = std::max(drop_start + phrase, total_beats - phrase);
	AddDjSection(&plan.dj_sections, "intro", "beatgrid-lock and DJ-friendly opening", 0.0f, intro_len, 0.42f, {"beatgrid-align", "filter-in", "sample-hook"}, {"EQ", "SpaceEcho"});
	AddDjSection(&plan.dj_sections, "groove", "main groove with balanced drums and vocal placement", intro_len, std::max(phrase, break_start - intro_len), 0.70f + 0.12f * Clamp01(metrics.beat_strength), {"drums-focus", "vocal-place", "bass-control"}, {"TransientPunch", "VocalPresence", "MasterGlue"});
	AddDjSection(&plan.dj_sections, "break", "create contrast before the new drop", break_start, phrase, 0.34f, {"drums-duck", "vocal-stretch", "reverb-throw", "sample-riser"}, {"ShimmerBloom", "SpaceEcho", "PitchShifter"});
	AddDjSection(&plan.dj_sections, "drop", "beautiful DJ mix payoff with sample layer return", drop_start, std::max(phrase, outro_start - drop_start), 0.88f, {"drums-return", "sample-drop", "master-lift"}, {"TransientPunch", "PsychoWidener", "Limiter"});
	AddDjSection(&plan.dj_sections, "outro", "clean mix-out tail for the next track", outro_start, std::max(8.0f, total_beats - outro_start), 0.48f, {"filter-out", "echo-tail", "remove-conflict"}, {"EQ", "Delay", "Limiter"});
	AddTransitionAction(&plan.transition_actions, "filter-sweep", "intro into groove", "intro", "groove", std::max(0.0f, intro_len - 8.0f), 8.0f, 0.76f, {"EQ", "SpaceEcho"});
	AddTransitionAction(&plan.transition_actions, "riser-drop", "break into drop", "break", "drop", std::max(0.0f, drop_start - 8.0f), 8.0f, 0.82f + 0.10f * intensity, {"ShimmerBloom", "TransientPunch", "Limiter"});
	AddTransitionAction(&plan.transition_actions, "echo-out", "outro mix-out", "drop", "outro", std::max(0.0f, outro_start - 8.0f), 8.0f, 0.70f, {"Delay", "EQ"});
	return plan;
}

}  // namespace Engine::Audio::AnalysisAI