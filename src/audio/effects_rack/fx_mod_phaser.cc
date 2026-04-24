#include "fx_mod_phaser.h"

#include <algorithm>
#include <cmath>

namespace Engine::Audio::FX {

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

ModPhaser::ModPhaser()
	: sample_rate_(44100.0f),
	  stages_(4),
	  rate_hz_(0.35f),
	  depth_(0.7f),
	  feedback_(0.2f),
	  mix_(0.5f),
	  lfo_phase_(0.0f),
	  feedback_state_(0.0f),
	  mutex_("fx_phaser") {}

ModPhaser::~ModPhaser() = default;

bool ModPhaser::Initialize(float sample_rate, size_t stages) {
	if (sample_rate <= 0.0f || stages == 0) {
		return false;
	}
	sample_rate_ = sample_rate;
	stages_ = stages;
	ap_state_.assign(stages_, 0.0f);
	lfo_phase_ = 0.0f;
	feedback_state_ = 0.0f;
	return true;
}

void ModPhaser::SetRateHz(float rate_hz) {
	rate_hz_ = std::max(0.01f, rate_hz);
}

void ModPhaser::SetDepth(float depth) {
	depth_ = std::clamp(depth, 0.0f, 1.0f);
}

void ModPhaser::SetFeedback(float feedback) {
	feedback_ = std::clamp(feedback, -0.95f, 0.95f);
}

void ModPhaser::SetMix(float mix) {
	mix_ = std::clamp(mix, 0.0f, 1.0f);
}

bool ModPhaser::ProcessBlock(const float* input, size_t frame_count, float* output) {
	if (input == nullptr || output == nullptr) {
		return false;
	}

	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	const float phase_inc = 2.0f * kPi * rate_hz_ / sample_rate_;

	for (size_t i = 0; i < frame_count; ++i) {
		const float lfo = (std::sin(lfo_phase_) + 1.0f) * 0.5f;
		const float a = 0.1f + lfo * depth_ * 0.8f;
		float x = input[i] + feedback_state_ * feedback_;

		for (size_t s = 0; s < stages_; ++s) {
			const float y = -a * x + ap_state_[s];
			ap_state_[s] = x + a * y;
			x = y;
		}

		feedback_state_ = x;
		output[i] = input[i] * (1.0f - mix_) + x * mix_;

		lfo_phase_ += phase_inc;
		if (lfo_phase_ > 2.0f * kPi) {
			lfo_phase_ -= 2.0f * kPi;
		}
	}
	return true;
}

std::string ModPhaser::GetReport() const {
	return "Phaser: stages=" + std::to_string(stages_) +
		", rate=" + std::to_string(rate_hz_) +
		", depth=" + std::to_string(depth_) +
		", mix=" + std::to_string(mix_);
}

}  // namespace Engine::Audio::FX
