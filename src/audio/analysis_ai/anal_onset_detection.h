#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "async_io/log_and_debug/io_perf_counter.h"
#include "async_io/sync_primitives/mutex_wrapper.h"

#include "audio_fft_analyzer.h"

namespace Engine::Audio::AnalysisAI {

class OnsetDetector {
public:
	OnsetDetector(size_t window_size = 1024, size_t hop_size = 256);
	~OnsetDetector();

	bool DetectOnsets(const float* audio, size_t frame_count, int sample_rate);
	void Reset();

	const std::vector<size_t>& GetOnsetFrames() const { return onset_frames_; }
	std::vector<double> GetOnsetTimesSeconds(int sample_rate) const;
	const std::vector<float>& GetFluxCurve() const { return flux_curve_; }
	float GetAverageStrength() const;
	std::string GetReport() const;

private:
	size_t window_size_;
	size_t hop_size_;
	AsyncIO::IO::LogDebug::PerformanceCounter perf_counter_;
	AsyncIO::IO::Sync::MutexWrapper mutex_;
	AudioFFTAnalyzer fft_analyzer_;
	std::vector<size_t> onset_frames_;
	std::vector<float> onset_strengths_;
	std::vector<float> flux_curve_;

	static float ComputeAdaptiveThreshold(const std::vector<float>& flux_curve, size_t index);
};

}  // namespace Engine::Audio::AnalysisAI
