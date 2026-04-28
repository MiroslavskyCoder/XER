#include "dsp_phase_vocoder.h"

#include <algorithm>
#include <cmath>

namespace Engine::Audio::DSP {

namespace {

constexpr float kTwoPi = 6.28318530717958647692f;

float WrapPhase(float phase) {
	while (phase > static_cast<float>(M_PI)) {
		phase -= kTwoPi;
	}
	while (phase < -static_cast<float>(M_PI)) {
		phase += kTwoPi;
	}
	return phase;
}

}  // namespace

PhaseVocoder::PhaseVocoder(size_t fft_size, size_t hop_size)
	: fft_size_(fft_size),
	  hop_size_(hop_size),
	  sample_rate_(44100.0f),
	  time_stretch_ratio_(1.0f),
	  fft_engine_(fft_size),
	  previous_phase_((fft_size / 2) + 1, 0.0f),
	  phase_accumulator_((fft_size / 2) + 1, 0.0f) {
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

	auto& spectrum = fft_engine_.MutableSpectrum();
	const size_t positive_bins = (fft_size_ / 2) + 1;
	const float analysis_hop = static_cast<float>(hop_size_);
	const float synthesis_hop = analysis_hop * time_stretch_ratio_;
	for (size_t bin = 0; bin < positive_bins; ++bin) {
		const float magnitude = std::abs(spectrum[bin]);
		const float phase = std::arg(spectrum[bin]);
		const float expected_advance = kTwoPi * static_cast<float>(bin) * analysis_hop / static_cast<float>(fft_size_);
		const float delta = WrapPhase(phase - previous_phase_[bin] - expected_advance);
		const float true_frequency = (kTwoPi * static_cast<float>(bin) / static_cast<float>(fft_size_)) + (delta / std::max(analysis_hop, 1.0f));

		phase_accumulator_[bin] += true_frequency * synthesis_hop;
		previous_phase_[bin] = phase;
		spectrum[bin] = std::polar(magnitude, phase_accumulator_[bin]);
	}

	for (size_t bin = positive_bins; bin < fft_size_; ++bin) {
		const size_t mirrored = fft_size_ - bin;
		spectrum[bin] = std::conj(spectrum[mirrored]);
	}

	if (!fft_engine_.Inverse(output, frame_count)) {
		perf_counter_.StopCounter("phase_vocoder");
		return false;
	}

	const float gain = 1.0f / std::sqrt(std::max(time_stretch_ratio_, 0.0001f));
	for (size_t i = 0; i < frame_count; ++i) {
		output[i] *= gain;
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
