#include "dsp_fir_filter_bank.h"

namespace Engine::Audio::DSP {

FIRFilterBank::FIRFilterBank()
	: mutex_("fir_filter_bank") {}

FIRFilterBank::~FIRFilterBank() = default;

void FIRFilterBank::Clear() {
	IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	filters_.clear();
	states_.clear();
}

bool FIRFilterBank::AddFilter(const std::vector<float>& taps) {
	if (taps.empty()) {
		return false;
	}

	IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	filters_.push_back(taps);
	states_.push_back(std::vector<float>(taps.size(), 0.0f));
	return true;
}

bool FIRFilterBank::ProcessSample(float input, std::vector<float>& outputs) {
	IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	outputs.assign(filters_.size(), 0.0f);

	for (size_t filter_index = 0; filter_index < filters_.size(); ++filter_index) {
		auto& state = states_[filter_index];
		const auto& taps = filters_[filter_index];

		state.insert(state.begin(), input);
		state.pop_back();

		float y = 0.0f;
		for (size_t tap = 0; tap < taps.size(); ++tap) {
			y += taps[tap] * state[tap];
		}
		outputs[filter_index] = y;
	}

	return true;
}

std::string FIRFilterBank::GetReport() const {
	return "FIRBank: filters=" + std::to_string(filters_.size());
}

}  // namespace Engine::Audio::DSP
