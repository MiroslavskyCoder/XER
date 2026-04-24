#pragma once

#include <cstddef>
#include <string>

#include "async_io/log_and_debug/io_perf_counter.h"
#include "audio/dsp_algorithms/dsp_delay_line_circular.h"

namespace Engine::Audio::FX {

class ReverbAlgorithmic {
public:
	ReverbAlgorithmic();
	~ReverbAlgorithmic();

	bool Initialize(float sample_rate, size_t max_delay_samples);
	void SetRoomSize(float room_size);
	void SetDamping(float damping);
	void SetMix(float mix);
	bool ProcessBlock(const float* input, size_t frame_count, float* output);

	std::string GetReport() const;

private:
	float sample_rate_;
	float room_size_;
	float damping_;
	float mix_;
	Engine::Audio::DSP::CircularDelayLine comb_a_;
	Engine::Audio::DSP::CircularDelayLine comb_b_;
	Engine::Audio::DSP::CircularDelayLine allpass_;
	IO::LogDebug::PerformanceCounter perf_counter_;
};

}  // namespace Engine::Audio::FX
