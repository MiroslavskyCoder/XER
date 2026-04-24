#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "async_io/sync_primitives/mutex_wrapper.h"

namespace Engine::Audio::DSP {

class FIRFilterBank {
public:
	FIRFilterBank();
	~FIRFilterBank();

	void Clear();
	bool AddFilter(const std::vector<float>& taps);
	size_t GetFilterCount() const { return filters_.size(); }

	bool ProcessSample(float input, std::vector<float>& outputs);
	std::string GetReport() const;

private:
	AsyncIO::IO::Sync::MutexWrapper mutex_;
	std::vector<std::vector<float>> filters_;
	std::vector<std::vector<float>> states_;
};

}  // namespace Engine::Audio::DSP
