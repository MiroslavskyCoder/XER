#include "anal_onset_detection.h"

#include <algorithm>
#include <numeric>
#include <vector>

namespace Engine::Audio::AnalysisAI {

OnsetDetector::OnsetDetector(size_t window_size, size_t hop_size)
	: window_size_(window_size),
	  hop_size_(hop_size),
	  mutex_("onset_detector"),
	  fft_analyzer_(window_size) {
	perf_counter_.Enable();
}

OnsetDetector::~OnsetDetector() = default;

bool OnsetDetector::DetectOnsets(const float* audio, size_t frame_count, int sample_rate) {
	if (audio == nullptr || frame_count < window_size_ || sample_rate <= 0) {
		return false;
	}

	IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	perf_counter_.StartCounter("onset_detection");

	onset_frames_.clear();
	onset_strengths_.clear();
	flux_curve_.clear();

	std::vector<float> previous_spectrum(window_size_ / 2, 0.0f);
	std::vector<float> window(window_size_, 0.0f);

	for (size_t offset = 0; offset + window_size_ <= frame_count; offset += hop_size_) {
		std::copy(audio + offset, audio + offset + window_size_, window.begin());
		if (!fft_analyzer_.AnalyzeSpectrum(window.data(), window.size())) {
			continue;
		}

		const std::vector<float>& magnitude = fft_analyzer_.GetMagnitudeSpectrum();
		float flux = 0.0f;
		for (size_t index = 0; index < magnitude.size(); ++index) {
			const float delta = magnitude[index] - previous_spectrum[index];
			if (delta > 0.0f) {
				flux += delta;
			}
			previous_spectrum[index] = magnitude[index];
		}

		flux_curve_.push_back(flux);
		const float threshold = ComputeAdaptiveThreshold(flux_curve_, flux_curve_.size() - 1);
		if (flux > threshold * 1.5f) {
			onset_frames_.push_back(offset);
			onset_strengths_.push_back(flux);
		}
	}

	perf_counter_.StopCounter("onset_detection");
	return true;
}

void OnsetDetector::Reset() {
	IO::Sync::MutexWrapper::ScopedLock lock(mutex_);
	onset_frames_.clear();
	onset_strengths_.clear();
	flux_curve_.clear();
}

std::vector<double> OnsetDetector::GetOnsetTimesSeconds(int sample_rate) const {
	std::vector<double> times;
	if (sample_rate <= 0) {
		return times;
	}

	times.reserve(onset_frames_.size());
	for (size_t frame : onset_frames_) {
		times.push_back(static_cast<double>(frame) / static_cast<double>(sample_rate));
	}
	return times;
}

float OnsetDetector::GetAverageStrength() const {
	if (onset_strengths_.empty()) {
		return 0.0f;
	}

	return std::accumulate(onset_strengths_.begin(), onset_strengths_.end(), 0.0f) /
		static_cast<float>(onset_strengths_.size());
}

std::string OnsetDetector::GetReport() const {
	return "OnsetDetector: onsets=" + std::to_string(onset_frames_.size()) +
		", avg_flux=" + std::to_string(GetAverageStrength());
}

float OnsetDetector::ComputeAdaptiveThreshold(const std::vector<float>& flux_curve, size_t index) {
	if (flux_curve.empty()) {
		return 0.0f;
	}

	const size_t begin = index > 8 ? index - 8 : 0;
	const auto begin_it = flux_curve.begin() + static_cast<std::ptrdiff_t>(begin);
	const auto end_it = flux_curve.begin() + static_cast<std::ptrdiff_t>(index + 1);
	const float local_mean = std::accumulate(begin_it, end_it, 0.0f) /
		static_cast<float>(index - begin + 1);
	return std::max(local_mean, 1e-6f);
}

}  // namespace Engine::Audio::AnalysisAI
