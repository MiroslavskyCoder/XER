#include "fx_eq_parametric.h"

#include <algorithm>
#include <cmath>

namespace Engine::Audio::FX {

namespace {
float DbToLinear(float db_value) {
	return std::pow(10.0f, db_value / 20.0f);
}
}

ParametricEQ::ParametricEQ()
	: sample_rate_(44100.0f),
	  mutex_("parametric_eq") {}

ParametricEQ::~ParametricEQ() = default;

bool ParametricEQ::Initialize(float sample_rate, size_t band_count) {
	if (sample_rate <= 0.0f || band_count == 0) {
		return false;
	}

	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	sample_rate_ = sample_rate;
	bands_.assign(band_count, ParametricBand{1000.0f, 1.0f, 0.0f, true});
	filters_.assign(band_count, Engine::Audio::DSP::BiquadProcessor());

	for (size_t i = 0; i < band_count; ++i) {
		filters_[i].Configure(Engine::Audio::DSP::BiquadType::BandPass, sample_rate_, bands_[i].frequency, bands_[i].q);
	}
	return true;
}

bool ParametricEQ::SetBand(size_t index, const ParametricBand& band) {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	if (index >= bands_.size()) {
		return false;
	}

	bands_[index] = band;
	return filters_[index].Configure(Engine::Audio::DSP::BiquadType::BandPass, sample_rate_, band.frequency, band.q);
}

ParametricBand ParametricEQ::GetBand(size_t index) const {
	if (index >= bands_.size()) {
		return ParametricBand{0.0f, 1.0f, 0.0f, false};
	}
	return bands_[index];
}

bool ParametricEQ::ProcessBlock(const float* input, size_t frame_count, float* output) {
	if (input == nullptr || output == nullptr) {
		return false;
	}

	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	for (size_t n = 0; n < frame_count; ++n) {
		float sum = input[n];
		for (size_t band = 0; band < bands_.size(); ++band) {
			if (!bands_[band].enabled) {
				continue;
			}

			const float filtered = filters_[band].ProcessSample(input[n]);
			sum += filtered * (DbToLinear(bands_[band].gain_db) - 1.0f);
		}
		output[n] = sum;
	}
	return true;
}

std::string ParametricEQ::GetReport() const {
	return "ParametricEQ: bands=" + std::to_string(bands_.size()) +
		", sr=" + std::to_string(sample_rate_);
}

}  // namespace Engine::Audio::FX
