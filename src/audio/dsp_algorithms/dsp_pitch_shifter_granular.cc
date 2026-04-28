#include "dsp_pitch_shifter_granular.h"

#include <algorithm>
#include <cmath>

namespace Engine::Audio::DSP {

GranularPitchShifter::GranularPitchShifter()
	: sample_rate_(44100.0f),
	  pitch_ratio_(1.0f),
	  grain_size_(256),
	  max_delay_samples_(0),
	  write_index_(0) {
	perf_counter_.Enable();
}

GranularPitchShifter::~GranularPitchShifter() = default;

bool GranularPitchShifter::Initialize(float sample_rate, size_t grain_size, size_t max_delay_samples) {
	if (sample_rate <= 0.0f || grain_size == 0 || max_delay_samples == 0) {
		return false;
	}

	sample_rate_ = sample_rate;
	grain_size_ = grain_size;
	max_delay_samples_ = std::max(max_delay_samples, grain_size_ * 2);
	write_index_ = 0;
	grain_window_ = WindowingFunctions::GenerateHann(grain_size_);
	delay_buffer_.assign(max_delay_samples_, 0.0f);
	grains_[0].position = 0.0;
	grains_[1].position = static_cast<double>(grain_size_) * 0.5;
	return !delay_buffer_.empty();
}

void GranularPitchShifter::SetPitchRatio(float ratio) {
	pitch_ratio_ = std::clamp(ratio, 0.5f, 2.0f);
}

bool GranularPitchShifter::ProcessBlock(const float* input, size_t frame_count, float* output) {
	if (input == nullptr || output == nullptr || frame_count == 0 || delay_buffer_.empty() || grain_window_.empty()) {
		return false;
	}

	perf_counter_.StartCounter("granular_pitch_shift");
	const double usable_delay = static_cast<double>(max_delay_samples_ > 2 ? max_delay_samples_ - 2 : 1);
	const double sweep = std::min(usable_delay * 0.75, std::max(8.0, static_cast<double>(grain_size_) * 0.85));
	const double base_delay = std::max(1.0, usable_delay - sweep);

	for (size_t i = 0; i < frame_count; ++i) {
		delay_buffer_[write_index_] = input[i];

		float mixed = 0.0f;
		float weight_sum = 0.0f;
		for (GrainState& grain : grains_) {
			const double normalized_phase = grain.position / static_cast<double>(grain_size_);
			const size_t window_index = std::min(
				grain_window_.size() - 1,
				static_cast<size_t>(grain.position) % grain_window_.size());
			const float window = grain_window_[window_index];
			const double delay = base_delay + (1.0 - normalized_phase) * sweep;
			mixed += ReadDelayedSample(delay) * window;
			weight_sum += window;
			AdvanceGrain(&grain);
		}

		output[i] = weight_sum > 1.0e-6f ? mixed / weight_sum : 0.0f;
		write_index_ = (write_index_ + 1) % delay_buffer_.size();
	}

	perf_counter_.StopCounter("granular_pitch_shift");
	return true;
}

void GranularPitchShifter::Reset() {
	write_index_ = 0;
	std::fill(delay_buffer_.begin(), delay_buffer_.end(), 0.0f);
	grains_[0].position = 0.0;
	grains_[1].position = static_cast<double>(grain_size_) * 0.5;
}

std::string GranularPitchShifter::GetReport() const {
	return "GranularPitchShifter: sr=" + std::to_string(sample_rate_) +
		", ratio=" + std::to_string(pitch_ratio_) +
		", grain=" + std::to_string(grain_size_) +
		", delay=" + std::to_string(max_delay_samples_) +
		", grains=2";
}

float GranularPitchShifter::ReadDelayedSample(double delay_samples) const {
	if (delay_buffer_.empty()) {
		return 0.0f;
	}

	const double clamped_delay = std::clamp(delay_samples, 1.0, static_cast<double>(delay_buffer_.size() - 2));
	double read_position = static_cast<double>(write_index_) - clamped_delay;
	while (read_position < 0.0) {
		read_position += static_cast<double>(delay_buffer_.size());
	}
	while (read_position >= static_cast<double>(delay_buffer_.size())) {
		read_position -= static_cast<double>(delay_buffer_.size());
	}

	const size_t index_a = static_cast<size_t>(read_position);
	const size_t index_b = (index_a + 1) % delay_buffer_.size();
	const float fraction = static_cast<float>(read_position - std::floor(read_position));
	return delay_buffer_[index_a] + (delay_buffer_[index_b] - delay_buffer_[index_a]) * fraction;
}

void GranularPitchShifter::AdvanceGrain(GrainState* grain) {
	if (grain == nullptr) {
		return;
	}

	grain->position += static_cast<double>(pitch_ratio_);
	while (grain->position >= static_cast<double>(grain_size_)) {
		grain->position -= static_cast<double>(grain_size_);
	}
}

}  // namespace Engine::Audio::DSP
