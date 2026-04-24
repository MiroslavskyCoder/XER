#include "anal_spectrogram_generator.h"

#include <algorithm>
#include <cstring>

namespace Engine::Audio::AnalysisAI {

SpectrogramGenerator::SpectrogramGenerator(size_t fft_size, size_t hop_size)
	: fft_size_(fft_size),
	  hop_size_(hop_size),
	  fft_analyzer_(fft_size),
	  buffer_pool_(65536, 2) {
	perf_counter_.Enable();
}

SpectrogramGenerator::~SpectrogramGenerator() = default;

bool SpectrogramGenerator::Generate(const float* audio, size_t frame_count) {
	if (audio == nullptr || frame_count < fft_size_) {
		return false;
	}

	perf_counter_.StartCounter("spectrogram_generate");
	magnitude_matrix_.clear();

	std::vector<float> window(fft_size_, 0.0f);
	for (size_t offset = 0; offset + fft_size_ <= frame_count; offset += hop_size_) {
		std::copy(audio + offset, audio + offset + fft_size_, window.begin());
		if (!fft_analyzer_.AnalyzeSpectrum(window.data(), window.size())) {
			continue;
		}
		magnitude_matrix_.push_back(fft_analyzer_.GetMagnitudeSpectrum());
	}

	perf_counter_.StopCounter("spectrogram_generate");
	return !magnitude_matrix_.empty();
}

bool SpectrogramGenerator::ExportAsRaw(const std::string& filepath) const {
	if (magnitude_matrix_.empty()) {
		return false;
	}

	std::vector<float> flat;
	for (const auto& row : magnitude_matrix_) {
		flat.insert(flat.end(), row.begin(), row.end());
	}

	return IO::LogDebug::DumpHelper::DumpMemoryToFile(
		filepath,
		reinterpret_cast<const uint8_t*>(flat.data()),
		flat.size() * sizeof(float));
}

void SpectrogramGenerator::Reset() {
	magnitude_matrix_.clear();
}

std::string SpectrogramGenerator::GetReport() const {
	return "Spectrogram: frames=" + std::to_string(GetFrameCount()) +
		", bins=" + std::to_string(GetBinCount());
}

}  // namespace AIToolsXPro::Audio::AnalysisAI
