#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "async_io/async_buffer_pool.h"
#include "async_io/log_and_debug/io_perf_counter.h"

namespace Engine::Audio::DSP {

class ConvolutionEngine {
public:
	ConvolutionEngine();
	~ConvolutionEngine();

	bool SetImpulseResponse(const float* ir, size_t ir_size);
	bool ProcessBlock(const float* input, size_t frame_count, float* output);
	void Reset();

	size_t GetImpulseSize() const { return impulse_response_.size(); }
	std::string GetReport() const;

private:
	std::vector<float> impulse_response_;
	std::vector<float> history_;
	IO::AsyncIO::AsyncBufferPool buffer_pool_;
	IO::LogDebug::PerformanceCounter perf_counter_;
};

}  // namespace Engine::Audio::DSP
