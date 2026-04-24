#include "dsp_pitch_shifter_granular.h"

#include <algorithm>

namespace Engine::Audio::DSP {

GranularPitchShifter::GranularPitchShifter()
	: sample_rate_(44100.0f),
	  pitch_ratio_(1.0f),
	  grain_size_(256),
	  grain_index_(0) {
	perf_counter_.Enable();
}

GranularPitchShifter::~GranularPitchShifter() = default;

bool GranularPitchShifter::Initialize(float sample_rate, size_t grain_size, size_t max_delay_samples) {
	if (sample_rate <= 0.0f || grain_size == 0 || max_delay_samples == 0) {
		return false;
	}

	sample_rate_ = sample_rate;
	grain_size_ = grain_size;
	grain_index_ = 0;
	grain_window_ = WindowingFunctions::GenerateHann(grain_size_);
	return delay_line_.Initialize(max_delay_samples);
}

void GranularPitchShifter::SetPitchRatio(float ratio) {
	pitch_ratio_ = std::clamp(ratio, 0.5f, 2.0f);
}

bool GranularPitchShifter::ProcessBlock(const float* input, size_t frame_count, float* output) {
	if (input == nullptr || output == nullptr || frame_count == 0) {
		return false;
	}

	perf_counter_.StartCounter("granular_pitch_shift");
	const size_t max_delay = static_cast<size_t>(std::max(1.0f, (1.0f - std::min(pitch_ratio_, 1.0f)) * grain_size_));
	delay_line_.SetDelaySamples(max_delay);

	for (size_t i = 0; i < frame_count; ++i) {
		const float delayed = delay_line_.Process(input[i]);
		const float window = grain_window_.empty() ? 1.0f : grain_window_[grain_index_ % grain_window_.size()];
		output[i] = delayed * window;
		++grain_index_;
	}

	perf_counter_.StopCounter("granular_pitch_shift");
	return true;
}

void GranularPitchShifter::Reset() {
	grain_index_ = 0;
	delay_line_.Reset();
}

std::string GranularPitchShifter::GetReport() const {
	return "GranularPitchShifter: sr=" + std::to_string(sample_rate_) +
		", ratio=" + std::to_string(pitch_ratio_) +
		", grain=" + std::to_string(grain_size_);
}

}  // namespace Engine::Audio::DSP
