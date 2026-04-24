#pragma once

#include <complex>
#include <cstddef>
#include <string>
#include <vector>

#include "async_io/log_and_debug/io_perf_counter.h"

namespace Engine::Audio::DSP {

class FFTEngine {
public:
	using Complex = std::complex<float>;

	explicit FFTEngine(size_t fft_size = 1024);
	~FFTEngine();

	bool Forward(const float* input, size_t frame_count);
	bool Inverse(float* output, size_t frame_count) const;
	void Reset();

	const std::vector<Complex>& GetSpectrum() const { return spectrum_; }
	std::vector<float> GetMagnitude() const;
	std::string GetReport() const;

private:
	size_t fft_size_;
	std::vector<Complex> spectrum_;
	AsyncIO::IO::LogDebug::PerformanceCounter perf_counter_;
};

}  // namespace Engine::Audio::DSP
