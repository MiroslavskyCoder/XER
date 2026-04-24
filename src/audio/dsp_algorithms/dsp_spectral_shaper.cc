#include "dsp_spectral_shaper.h"

#include <algorithm>

namespace Engine::Audio::DSP {

SpectralShaper::SpectralShaper(size_t fft_size)
	: fft_size_(fft_size),
	  fft_engine_(fft_size),
	  shaping_curve_(fft_size, 1.0f),
	  mutex_("spectral_shaper") {}

SpectralShaper::~SpectralShaper() = default;

bool SpectralShaper::SetCurve(const std::vector<float>& curve) {
	if (curve.empty()) {
		return false;
	}

	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	shaping_curve_ = curve;
	return true;
}

bool SpectralShaper::ProcessBlock(const float* input, size_t frame_count, float* output) {
	if (input == nullptr || output == nullptr || frame_count != fft_size_) {
		return false;
	}

	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	if (!fft_engine_.Forward(input, frame_count)) {
		return false;
	}

	std::vector<float> magnitude = fft_engine_.GetMagnitude();
	const size_t limit = std::min(magnitude.size(), shaping_curve_.size());
	for (size_t i = 0; i < limit; ++i) {
		magnitude[i] *= shaping_curve_[i];
	}

	// Placeholder synthesis path.
	return fft_engine_.Inverse(output, frame_count);
}

void SpectralShaper::Reset() {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	std::fill(shaping_curve_.begin(), shaping_curve_.end(), 1.0f);
}

std::string SpectralShaper::GetReport() const {
	return "SpectralShaper: fft=" + std::to_string(fft_size_) +
		", curve_size=" + std::to_string(shaping_curve_.size());
}

}  // namespace Engine::Audio::DSP
