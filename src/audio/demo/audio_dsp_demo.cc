#include "audio/demo/audio_dsp_demo.h"

#include "audio/dsp_algorithms/dsp_phase_vocoder.h"
#include "audio/dsp_algorithms/dsp_spectral_shaper.h"
#include "audio/file_io_codecs/codec_wav_pcm.h"

#include <absl/strings/str_format.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <vector>

namespace Engine::Audio::Demo {

namespace {

constexpr float kSpectralShaperRmsThreshold = 1.0e-3f;
constexpr float kSpectralShaperPeakThreshold = 1.0e-2f;
constexpr float kPhaseVocoderRmsThreshold = 2.0e-3f;
constexpr float kPhaseVocoderPeakThreshold = 5.0e-3f;

struct ErrorMetrics {
	float rms = 0.0f;
	float peak = 0.0f;
};

bool WriteBinaryFile(
	const std::filesystem::path& path,
	const std::vector<std::uint8_t>& bytes,
	std::string* error_out) {
	std::ofstream output(path, std::ios::binary);
	if (!output.is_open()) {
		if (error_out != nullptr) {
			*error_out = absl::StrFormat("failed to open '%s'", path.string());
		}
		return false;
	}

	output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
	output.close();
	if (!output) {
		if (error_out != nullptr) {
			*error_out = absl::StrFormat("failed to write '%s'", path.string());
		}
		return false;
	}
	return true;
}

std::vector<float> GenerateInputSignal(size_t sample_count, int sample_rate) {
	std::vector<float> samples(sample_count, 0.0f);
	for (size_t i = 0; i < sample_count; ++i) {
		const float t = static_cast<float>(i) / static_cast<float>(sample_rate);
		samples[i] = 0.45f * std::sin(2.0f * static_cast<float>(M_PI) * 220.0f * t)
			+ 0.30f * std::sin(2.0f * static_cast<float>(M_PI) * 440.0f * t)
			+ 0.15f * std::sin(2.0f * static_cast<float>(M_PI) * 880.0f * t);
	}
	return samples;
}

ErrorMetrics ComputeErrorMetrics(
	const std::vector<float>& reference,
	const std::vector<float>& processed,
	size_t latency_samples) {
	ErrorMetrics metrics;
	if (processed.size() <= latency_samples || reference.empty()) {
		return metrics;
	}

	const size_t comparable = std::min(reference.size(), processed.size() - latency_samples);
	double sum_squared = 0.0;
	for (size_t i = 0; i < comparable; ++i) {
		const float error = processed[i + latency_samples] - reference[i];
		sum_squared += static_cast<double>(error) * static_cast<double>(error);
		metrics.peak = std::max(metrics.peak, std::abs(error));
	}
	metrics.rms = static_cast<float>(std::sqrt(sum_squared / static_cast<double>(comparable)));
	return metrics;
}

template <typename Processor>
std::vector<float> ProcessSignal(
	Processor* processor,
	const std::vector<float>& input,
	size_t hop_size,
	size_t latency_samples,
	std::string* error_out) {
	const size_t padded_size = input.size() + latency_samples;
	std::vector<float> padded_input(padded_size, 0.0f);
	std::copy(input.begin(), input.end(), padded_input.begin());

	std::vector<float> output(padded_size, 0.0f);
	for (size_t offset = 0; offset < padded_size; offset += hop_size) {
		if (!processor->ProcessBlock(padded_input.data() + static_cast<std::ptrdiff_t>(offset), hop_size, output.data() + static_cast<std::ptrdiff_t>(offset))) {
			if (error_out != nullptr) {
				*error_out = "audio dsp processor failed";
			}
			return {};
		}
	}
	return output;
}

bool EncodeWave(
	const std::filesystem::path& path,
	const std::vector<float>& audio,
	std::string* error_out) {
	Engine::Audio::CodecIO::WavPcmCodec codec;
	std::vector<std::uint8_t> encoded;
	if (!codec.Encode16(audio.data(), audio.size(), encoded)) {
		if (error_out != nullptr) {
			*error_out = "failed to encode wav";
		}
		return false;
	}
	return WriteBinaryFile(path, encoded, error_out);
}

}  // namespace

bool RunAudioDSPDemo(
	const std::filesystem::path& output_dir,
	std::string* report_out,
	std::string* error_out) {
	if (report_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio demo report target is null";
		}
		return false;
	}

	constexpr int kSampleRate = 44100;
	constexpr size_t kFFTSize = 1024;
	constexpr size_t kHopSize = 256;
	constexpr size_t kSignalSamples = kHopSize * 64;

	std::error_code fs_error;
	std::filesystem::create_directories(output_dir, fs_error);
	if (fs_error) {
		if (error_out != nullptr) {
			*error_out = "failed to create audio demo output directory";
		}
		return false;
	}

	const std::vector<float> input = GenerateInputSignal(kSignalSamples, kSampleRate);

	Engine::Audio::DSP::SpectralShaper shaper(kFFTSize, kHopSize);
	if (!shaper.SetCurve(std::vector<float>((kFFTSize / 2) + 1, 1.0f))) {
		if (error_out != nullptr) {
			*error_out = "failed to set spectral shaper curve";
		}
		return false;
	}

	Engine::Audio::DSP::PhaseVocoder vocoder(kFFTSize, kHopSize);
	if (!vocoder.Initialize(static_cast<float>(kSampleRate))) {
		if (error_out != nullptr) {
			*error_out = "failed to initialize phase vocoder";
		}
		return false;
	}
	vocoder.SetTimeStretchRatio(1.0f);

	const size_t latency_samples = kFFTSize - kHopSize;
	std::string processing_error;
	const std::vector<float> shaper_output = ProcessSignal(&shaper, input, kHopSize, latency_samples, &processing_error);
	if (shaper_output.empty()) {
		if (error_out != nullptr) {
			*error_out = processing_error;
		}
		return false;
	}

	const std::vector<float> vocoder_output = ProcessSignal(&vocoder, input, kHopSize, latency_samples, &processing_error);
	if (vocoder_output.empty()) {
		if (error_out != nullptr) {
			*error_out = processing_error;
		}
		return false;
	}

	const ErrorMetrics shaper_metrics = ComputeErrorMetrics(input, shaper_output, latency_samples);
	const ErrorMetrics vocoder_metrics = ComputeErrorMetrics(input, vocoder_output, latency_samples);
	const bool spectral_shaper_ok = shaper_metrics.rms <= kSpectralShaperRmsThreshold
		&& shaper_metrics.peak <= kSpectralShaperPeakThreshold;
	const bool phase_vocoder_ok = vocoder_metrics.rms <= kPhaseVocoderRmsThreshold
		&& vocoder_metrics.peak <= kPhaseVocoderPeakThreshold;
	const bool smoke_ok = spectral_shaper_ok && phase_vocoder_ok;

	const std::filesystem::path input_path = output_dir / "input.wav";
	const std::filesystem::path shaper_path = output_dir / "spectral_shaper_unity.wav";
	const std::filesystem::path vocoder_path = output_dir / "phase_vocoder_unity.wav";
	if (!EncodeWave(input_path, input, error_out)
		|| !EncodeWave(shaper_path, shaper_output, error_out)
		|| !EncodeWave(vocoder_path, vocoder_output, error_out)) {
		return false;
	}

	*report_out = absl::StrFormat(
		"Audio DSP demo\n"
		"smoke_status=%s\n"
		"sample_rate=%d\n"
		"fft_size=%zu\n"
		"hop_size=%zu\n"
		"latency_samples=%zu\n"
		"spectral_shaper_status=%s\n"
		"spectral_shaper_rms=%0.6f\n"
		"spectral_shaper_peak=%0.6f\n"
		"phase_vocoder_status=%s\n"
		"phase_vocoder_rms=%0.6f\n"
		"phase_vocoder_peak=%0.6f\n"
		"input_wav=%s\n"
		"spectral_shaper_wav=%s\n"
		"phase_vocoder_wav=%s\n",
		smoke_ok ? "pass" : "fail",
		kSampleRate,
		kFFTSize,
		kHopSize,
		latency_samples,
		spectral_shaper_ok ? "pass" : "fail",
		shaper_metrics.rms,
		shaper_metrics.peak,
		phase_vocoder_ok ? "pass" : "fail",
		vocoder_metrics.rms,
		vocoder_metrics.peak,
		input_path.string(),
		shaper_path.string(),
		vocoder_path.string());

	if (!smoke_ok) {
		if (error_out != nullptr) {
			*error_out = "audio dsp smoke thresholds exceeded";
		}
		return false;
	}

	if (error_out != nullptr) {
		error_out->clear();
	}
	return true;
}

}  // namespace Engine::Audio::Demo