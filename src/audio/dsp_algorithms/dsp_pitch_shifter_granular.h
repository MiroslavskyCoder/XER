#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "async_io/log_and_debug/io_perf_counter.h"
#include "dsp_delay_line_circular.h"
#include "dsp_windowing_functions.h"

namespace Engine::Audio::DSP {

class GranularPitchShifter {
public:
	GranularPitchShifter();
	~GranularPitchShifter();

	bool Initialize(float sample_rate, size_t grain_size, size_t max_delay_samples);
	void SetPitchRatio(float ratio);
	float GetPitchRatio() const { return pitch_ratio_; }

	bool ProcessBlock(const float* input, size_t frame_count, float* output);
	void Reset();
	std::string GetReport() const;

private:
	float sample_rate_;
	float pitch_ratio_;
	size_t grain_size_;
	size_t grain_index_;
	std::vector<float> grain_window_;
	CircularDelayLine delay_line_;
	AsyncIO::IO::LogDebug::PerformanceCounter perf_counter_;
};

}  // namespace Engine::Audio::DSP
