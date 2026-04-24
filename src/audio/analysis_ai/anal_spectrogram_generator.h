#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "async_io/async_buffer_pool.h"
#include "async_io/log_and_debug/io_dump_helper.h"
#include "async_io/log_and_debug/io_perf_counter.h"

#include "audio_fft_analyzer.h"

namespace Engine::Audio::AnalysisAI {

class SpectrogramGenerator {
public:
	SpectrogramGenerator(size_t fft_size = 1024, size_t hop_size = 256);
	~SpectrogramGenerator();

	bool Generate(const float* audio, size_t frame_count);
	bool ExportAsRaw(const std::string& filepath) const;
	void Reset();

	const std::vector<std::vector<float>>& GetMagnitudeMatrix() const { return magnitude_matrix_; }
	size_t GetFrameCount() const { return magnitude_matrix_.size(); }
	size_t GetBinCount() const { return fft_size_ / 2; }
	std::string GetReport() const;

private:
	size_t fft_size_;
	size_t hop_size_;
	AudioFFTAnalyzer fft_analyzer_;
	IO::AsyncIO::AsyncBufferPool buffer_pool_;
	AsyncIO::IO::LogDebug::PerformanceCounter perf_counter_;
	std::vector<std::vector<float>> magnitude_matrix_;
};

}  // namespace Engine::Audio::AnalysisAI
