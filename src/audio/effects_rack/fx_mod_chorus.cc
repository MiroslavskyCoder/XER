#include "fx_mod_chorus.h"

#include <algorithm>
#include <cmath>

namespace Engine::Audio::FX {

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

ModChorus::ModChorus()
	: sample_rate_(44100.0f),
	  rate_hz_(0.6f),
	  depth_samples_(18.0f),
	  mix_(0.35f),
	  lfo_phase_(0.0f) {
	perf_counter_.Enable();
}

ModChorus::~ModChorus() = default;

bool ModChorus::Initialize(float sample_rate, size_t max_delay_samples) {
	if (sample_rate <= 0.0f || max_delay_samples == 0) {
		return false;
	}
	sample_rate_ = sample_rate;
	lfo_phase_ = 0.0f;
	return delay_line_.Initialize(max_delay_samples);
}

void ModChorus::SetRateHz(float rate_hz) {
	rate_hz_ = std::max(0.01f, rate_hz);
}

void ModChorus::SetDepthSamples(float depth_samples) {
	depth_samples_ = std::max(1.0f, depth_samples);
}

void ModChorus::SetMix(float mix) {
	mix_ = std::clamp(mix, 0.0f, 1.0f);
}

bool ModChorus::ProcessBlock(const float* input, size_t frame_count, float* output) {
	if (input == nullptr || output == nullptr) {
		return false;
	}

	perf_counter_.StartCounter("fx_chorus");
	const float phase_inc = 2.0f * kPi * rate_hz_ / sample_rate_;

	for (size_t i = 0; i < frame_count; ++i) {
		const float lfo = (std::sin(lfo_phase_) + 1.0f) * 0.5f;
		const size_t delay = static_cast<size_t>(std::max(1.0f, lfo * depth_samples_));
		delay_line_.SetDelaySamples(delay);

		const float wet = delay_line_.Process(input[i]);
		output[i] = input[i] * (1.0f - mix_) + wet * mix_;

		lfo_phase_ += phase_inc;
		if (lfo_phase_ > 2.0f * kPi) {
			lfo_phase_ -= 2.0f * kPi;
		}
	}

	perf_counter_.StopCounter("fx_chorus");
	return true;
}

std::string ModChorus::GetReport() const {
	return "Chorus: rate=" + std::to_string(rate_hz_) +
		", depth=" + std::to_string(depth_samples_) +
		", mix=" + std::to_string(mix_);
}

}  // namespace Engine::Audio::FX
