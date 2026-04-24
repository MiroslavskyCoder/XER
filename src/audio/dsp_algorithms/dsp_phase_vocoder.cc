#include "dsp_phase_vocoder.h"

#include <algorithm>

namespace Engine::Audio::DSP {

PhaseVocoder::PhaseVocoder(size_t fft_size, size_t hop_size)
	: fft_size_(fft_size),
	  hop_size_(hop_size),
	  sample_rate_(44100.0f),
	  time_stretch_ratio_(1.0f),
	  fft_engine_(fft_size),
	  previous_phase_(fft_size / 2, 0.0f),
	  phase_accumulator_(fft_size / 2, 0.0f) {
	perf_counter_.Enable();
}

PhaseVocoder::~PhaseVocoder() = default;

bool PhaseVocoder::Initialize(float sample_rate) {
	if (sample_rate <= 0.0f) {
		return false;
	}
	sample_rate_ = sample_rate;
	Reset();
	return true;
}

void PhaseVocoder::SetTimeStretchRatio(float ratio) {
	if (ratio < 0.25f) {
		time_stretch_ratio_ = 0.25f;
	} else if (ratio > 4.0f) {
		time_stretch_ratio_ = 4.0f;
	} else {
		time_stretch_ratio_ = ratio;
	}
}

bool PhaseVocoder::ProcessBlock(const float* input, size_t frame_count, float* output) {
	if (input == nullptr || output == nullptr || frame_count != fft_size_) {
		return false;
	}

	perf_counter_.StartCounter("phase_vocoder");

	if (!fft_engine_.Forward(input, frame_count)) {
		perf_counter_.StopCounter("phase_vocoder");
		return false;
	}

	// Simplified placeholder: preserve spectrum and synthesize by inverse FFT.
	if (!fft_engine_.Inverse(output, frame_count)) {
		perf_counter_.StopCounter("phase_vocoder");
		return false;
	}

	if (time_stretch_ratio_ != 1.0f) {
		const float gain = 1.0f / std::max(time_stretch_ratio_, 0.0001f);
		for (size_t i = 0; i < frame_count; ++i) {
			output[i] *= gain;
		}
	}

	perf_counter_.StopCounter("phase_vocoder");
	return true;
}

void PhaseVocoder::Reset() {
	std::fill(previous_phase_.begin(), previous_phase_.end(), 0.0f);
	std::fill(phase_accumulator_.begin(), phase_accumulator_.end(), 0.0f);
}

std::string PhaseVocoder::GetReport() const {
	return "PhaseVocoder: fft=" + std::to_string(fft_size_) +
		", hop=" + std::to_string(hop_size_) +
		", stretch=" + std::to_string(time_stretch_ratio_);
}

}  // namespace Engine::Audio::DSP
