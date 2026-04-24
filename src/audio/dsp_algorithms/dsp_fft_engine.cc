#include "dsp_fft_engine.h"

#include <cmath>

namespace Engine::Audio::DSP {

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

FFTEngine::FFTEngine(size_t fft_size)
	: fft_size_(fft_size), spectrum_(fft_size) {
	perf_counter_.Enable();
}

FFTEngine::~FFTEngine() = default;

bool FFTEngine::Forward(const float* input, size_t frame_count) {
	if (input == nullptr || frame_count != fft_size_) {
		return false;
	}

	perf_counter_.StartCounter("fft_forward");
	for (size_t k = 0; k < fft_size_; ++k) {
		Complex sum(0.0f, 0.0f);
		for (size_t n = 0; n < fft_size_; ++n) {
			const float angle = -2.0f * kPi * static_cast<float>(k * n) / static_cast<float>(fft_size_);
			sum += input[n] * Complex(std::cos(angle), std::sin(angle));
		}
		spectrum_[k] = sum;
	}
	perf_counter_.StopCounter("fft_forward");
	return true;
}

bool FFTEngine::Inverse(float* output, size_t frame_count) const {
	if (output == nullptr || frame_count != fft_size_) {
		return false;
	}

	for (size_t n = 0; n < fft_size_; ++n) {
		Complex sum(0.0f, 0.0f);
		for (size_t k = 0; k < fft_size_; ++k) {
			const float angle = 2.0f * kPi * static_cast<float>(k * n) / static_cast<float>(fft_size_);
			sum += spectrum_[k] * Complex(std::cos(angle), std::sin(angle));
		}
		output[n] = sum.real() / static_cast<float>(fft_size_);
	}
	return true;
}

void FFTEngine::Reset() {
	for (Complex& value : spectrum_) {
		value = Complex(0.0f, 0.0f);
	}
}

std::vector<float> FFTEngine::GetMagnitude() const {
	std::vector<float> magnitude(spectrum_.size(), 0.0f);
	for (size_t i = 0; i < spectrum_.size(); ++i) {
		magnitude[i] = std::abs(spectrum_[i]);
	}
	return magnitude;
}

std::string FFTEngine::GetReport() const {
	return "FFTEngine: size=" + std::to_string(fft_size_);
}

}  // namespace Audio::DSP
