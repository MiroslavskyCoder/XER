#include "dsp_iir_filter_bank.h"

namespace Engine::Audio::DSP {

IIRFilterBank::IIRFilterBank()
	: mutex_("iir_filter_bank") {}

IIRFilterBank::~IIRFilterBank() = default;

void IIRFilterBank::Clear() {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	filters_.clear();
}

bool IIRFilterBank::AddBiquad(BiquadType type, float sample_rate, float frequency, float q) {
	BiquadProcessor biquad;
	if (!biquad.Configure(type, sample_rate, frequency, q)) {
		return false;
	}

	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	filters_.push_back(biquad);
	return true;
}

bool IIRFilterBank::ProcessSample(float input, std::vector<float>& outputs) {
	AsyncIO::IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	outputs.assign(filters_.size(), 0.0f);

	for (size_t index = 0; index < filters_.size(); ++index) {
		outputs[index] = filters_[index].ProcessSample(input);
	}

	return true;
}

std::string IIRFilterBank::GetReport() const {
	return "IIRBank: filters=" + std::to_string(filters_.size());
}

}  // namespace Engine::Audio::DSP
