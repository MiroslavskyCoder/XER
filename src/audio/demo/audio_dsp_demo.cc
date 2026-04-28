#include "audio/demo/audio_dsp_demo.h"

#include "audio/dsp_algorithms/dsp_phase_vocoder.h"
#include "audio/dsp_algorithms/dsp_spectral_shaper.h"
#include "audio/file_io_codecs/codec_wav_pcm.h"
#include "wrapper/ffmpeg/ffmpeg_engine_bridge.h"

#include <absl/strings/str_format.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <vector>

namespace Engine::Audio::Demo {

namespace {

constexpr int kDefaultSampleRate = 44100;
constexpr size_t kFFTSize = 1024;
constexpr size_t kHopSize = 256;
constexpr size_t kSyntheticSignalSamples = kHopSize * 64;
constexpr float kSpectralShaperRmsThreshold = 1.0e-3f;
constexpr float kSpectralShaperPeakThreshold = 1.0e-2f;
constexpr float kPhaseVocoderRmsThreshold = 2.0e-3f;
constexpr float kPhaseVocoderPeakThreshold = 5.0e-3f;

struct ErrorMetrics {
	float rms = 0.0f;
	float peak = 0.0f;
};

struct DecodedAudio {
	std::vector<float> samples;
	int sample_rate = 0;
	std::string source_format;
	std::string codec_name;
	std::string decode_backend;
};

enum class DemoProcessorKind {
	kSpectralShaper,
	kPhaseVocoder,
};

std::string ToLowerCopy(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return value;
}

std::string CanonicalSampleFormat(std::string sample_format) {
	sample_format = ToLowerCopy(std::move(sample_format));
	if (!sample_format.empty() && sample_format.back() == 'p') {
		sample_format.pop_back();
	}
	return sample_format;
}

bool ReadBinaryFile(
	const std::filesystem::path& path,
	std::vector<std::uint8_t>* bytes_out,
	std::string* error_out) {
	if (bytes_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "byte target is null";
		}
		return false;
	}

	std::ifstream input(path, std::ios::binary | std::ios::ate);
	if (!input.is_open()) {
		if (error_out != nullptr) {
			*error_out = absl::StrFormat("failed to open '%s'", path.string());
		}
		return false;
	}

	const std::streamsize size = input.tellg();
	if (size < 0) {
		if (error_out != nullptr) {
			*error_out = absl::StrFormat("failed to size '%s'", path.string());
		}
		return false;
	}

	bytes_out->assign(static_cast<size_t>(size), 0);
	input.seekg(0, std::ios::beg);
	if (size > 0) {
		input.read(reinterpret_cast<char*>(bytes_out->data()), size);
	}
	if (!input) {
		if (error_out != nullptr) {
			*error_out = absl::StrFormat("failed to read '%s'", path.string());
		}
		return false;
	}
	return true;
}

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

std::vector<float> TrimLatency(
	const std::vector<float>& processed,
	size_t latency_samples,
	size_t target_frames) {
	if (processed.size() <= latency_samples) {
		return {};
	}

	const size_t available_frames = processed.size() - latency_samples;
	const size_t output_frames = target_frames == 0 ? available_frames : std::min(target_frames, available_frames);
	return std::vector<float>(
		processed.begin() + static_cast<std::ptrdiff_t>(latency_samples),
		processed.begin() + static_cast<std::ptrdiff_t>(latency_samples + output_frames));
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
	if (processor == nullptr || hop_size == 0) {
		if (error_out != nullptr) {
			*error_out = "audio dsp processor is not configured";
		}
		return {};
	}

	const size_t base_size = input.size() + latency_samples;
	const size_t padded_size = ((base_size + hop_size - 1) / hop_size) * hop_size;
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
	int sample_rate,
	std::string* error_out) {
	Engine::Audio::CodecIO::WavPcmCodec codec;
	std::vector<std::uint8_t> encoded;
	if (!codec.Encode16(audio.data(), audio.size(), encoded, sample_rate)) {
		if (error_out != nullptr) {
			*error_out = "failed to encode wav";
		}
		return false;
	}
	return WriteBinaryFile(path, encoded, error_out);
}

size_t BytesPerSample(const std::string& sample_format) {
	const std::string canonical = CanonicalSampleFormat(sample_format);
	if (canonical == "u8") {
		return 1;
	}
	if (canonical == "s16") {
		return 2;
	}
	if (canonical == "s32" || canonical == "flt") {
		return 4;
	}
	if (canonical == "s64" || canonical == "dbl") {
		return 8;
	}
	return 0;
}

template <typename SampleType>
SampleType ReadPod(const std::uint8_t* data) {
	SampleType value{};
	std::memcpy(&value, data, sizeof(SampleType));
	return value;
}

bool DecodeSample(const std::string& sample_format, const std::uint8_t* data, float* sample_out) {
	if (data == nullptr || sample_out == nullptr) {
		return false;
	}

	const std::string canonical = CanonicalSampleFormat(sample_format);
	if (canonical == "u8") {
		*sample_out = (static_cast<float>(*data) - 128.0f) / 128.0f;
		return true;
	}
	if (canonical == "s16") {
		*sample_out = static_cast<float>(ReadPod<std::int16_t>(data)) / 32768.0f;
		return true;
	}
	if (canonical == "s32") {
		*sample_out = static_cast<float>(static_cast<double>(ReadPod<std::int32_t>(data)) / 2147483648.0);
		return true;
	}
	if (canonical == "s64") {
		*sample_out = static_cast<float>(static_cast<long double>(ReadPod<std::int64_t>(data)) / 9223372036854775808.0L);
		return true;
	}
	if (canonical == "flt") {
		*sample_out = ReadPod<float>(data);
		return true;
	}
	if (canonical == "dbl") {
		*sample_out = static_cast<float>(ReadPod<double>(data));
		return true;
	}
	return false;
}

bool AppendFrameAsMono(
	const engine::bridge::ffmpeg::AudioFrameInfo& frame,
	std::vector<float>* samples_out,
	std::string* error_out) {
	if (samples_out == nullptr || frame.channels <= 0 || frame.sample_count <= 0) {
		if (error_out != nullptr) {
			*error_out = "invalid decoded audio frame";
		}
		return false;
	}

	const size_t bytes_per_sample = BytesPerSample(frame.sample_format);
	if (bytes_per_sample == 0) {
		if (error_out != nullptr) {
			*error_out = "unsupported ffmpeg sample format: " + frame.sample_format;
		}
		return false;
	}

	if (frame.planar) {
		if (frame.plane_sizes.size() < static_cast<size_t>(frame.channels)) {
			if (error_out != nullptr) {
				*error_out = "planar audio frame is missing channel planes";
			}
			return false;
		}

		std::vector<size_t> plane_offsets(static_cast<size_t>(frame.channels), 0);
		size_t cursor = 0;
		for (int channel = 0; channel < frame.channels; ++channel) {
			const size_t plane_size = static_cast<size_t>(frame.plane_sizes[static_cast<size_t>(channel)]);
			const size_t minimum_plane_size = static_cast<size_t>(frame.sample_count) * bytes_per_sample;
			if (plane_size < minimum_plane_size || cursor + plane_size > frame.data.size()) {
				if (error_out != nullptr) {
					*error_out = "planar audio frame payload is truncated";
				}
				return false;
			}
			plane_offsets[static_cast<size_t>(channel)] = cursor;
			cursor += plane_size;
		}

		samples_out->reserve(samples_out->size() + static_cast<size_t>(frame.sample_count));
		for (int sample_index = 0; sample_index < frame.sample_count; ++sample_index) {
			float mixed_sample = 0.0f;
			for (int channel = 0; channel < frame.channels; ++channel) {
				float decoded_sample = 0.0f;
				const size_t byte_offset = plane_offsets[static_cast<size_t>(channel)]
					+ static_cast<size_t>(sample_index) * bytes_per_sample;
				if (!DecodeSample(frame.sample_format, frame.data.data() + static_cast<std::ptrdiff_t>(byte_offset), &decoded_sample)) {
					if (error_out != nullptr) {
						*error_out = "failed to decode planar audio sample";
					}
					return false;
				}
				mixed_sample += decoded_sample;
			}
			samples_out->push_back(mixed_sample / static_cast<float>(frame.channels));
		}
		return true;
	}

	const size_t required_bytes = static_cast<size_t>(frame.sample_count)
		* static_cast<size_t>(frame.channels)
		* bytes_per_sample;
	if (frame.data.size() < required_bytes) {
		if (error_out != nullptr) {
			*error_out = "interleaved audio frame payload is truncated";
		}
		return false;
	}

	samples_out->reserve(samples_out->size() + static_cast<size_t>(frame.sample_count));
	for (int sample_index = 0; sample_index < frame.sample_count; ++sample_index) {
		float mixed_sample = 0.0f;
		for (int channel = 0; channel < frame.channels; ++channel) {
			float decoded_sample = 0.0f;
			const size_t byte_offset = (static_cast<size_t>(sample_index) * static_cast<size_t>(frame.channels)
				+ static_cast<size_t>(channel)) * bytes_per_sample;
			if (!DecodeSample(frame.sample_format, frame.data.data() + static_cast<std::ptrdiff_t>(byte_offset), &decoded_sample)) {
				if (error_out != nullptr) {
					*error_out = "failed to decode interleaved audio sample";
				}
				return false;
			}
			mixed_sample += decoded_sample;
		}
		samples_out->push_back(mixed_sample / static_cast<float>(frame.channels));
	}
	return true;
}

bool DecodeRawPcmFile(
	const std::filesystem::path& path,
	int sample_rate,
	DecodedAudio* decoded_audio,
	std::string* error_out) {
	if (decoded_audio == nullptr || sample_rate <= 0) {
		if (error_out != nullptr) {
			*error_out = "raw pcm decode target is invalid";
		}
		return false;
	}

	std::vector<std::uint8_t> bytes;
	if (!ReadBinaryFile(path, &bytes, error_out)) {
		return false;
	}
	if (bytes.empty() || (bytes.size() % 2u) != 0u) {
		if (error_out != nullptr) {
			*error_out = "raw pcm input must contain 16-bit mono samples";
		}
		return false;
	}

	decoded_audio->samples.resize(bytes.size() / 2u, 0.0f);
	for (size_t index = 0; index < decoded_audio->samples.size(); ++index) {
		const std::int16_t sample = static_cast<std::int16_t>(
			static_cast<std::uint16_t>(bytes[index * 2u])
			| (static_cast<std::uint16_t>(bytes[index * 2u + 1u]) << 8u));
		decoded_audio->samples[index] = static_cast<float>(sample) / 32768.0f;
	}
	decoded_audio->sample_rate = sample_rate;
	decoded_audio->source_format = "raw_s16le_mono";
	decoded_audio->codec_name = "pcm_s16le";
	decoded_audio->decode_backend = "manual";
	return true;
}

bool DecodeWavPcmFile(
	const std::filesystem::path& path,
	DecodedAudio* decoded_audio,
	std::string* error_out) {
	if (decoded_audio == nullptr) {
		if (error_out != nullptr) {
			*error_out = "wav decode target is invalid";
		}
		return false;
	}

	std::vector<std::uint8_t> bytes;
	if (!ReadBinaryFile(path, &bytes, error_out)) {
		return false;
	}

	Engine::Audio::CodecIO::WavPcmCodec codec;
	int sample_rate = 0;
	if (!codec.Decode16(bytes.data(), bytes.size(), decoded_audio->samples, &sample_rate)) {
		if (error_out != nullptr) {
			*error_out = "failed to decode PCM WAV input";
		}
		return false;
	}

	decoded_audio->sample_rate = sample_rate > 0 ? sample_rate : kDefaultSampleRate;
	decoded_audio->source_format = "wav";
	decoded_audio->codec_name = "pcm_s16le";
	decoded_audio->decode_backend = "local_wav";
	return true;
}

bool DecodeWithFFmpeg(
	const std::filesystem::path& path,
	DecodedAudio* decoded_audio,
	std::string* error_out) {
	if (decoded_audio == nullptr) {
		if (error_out != nullptr) {
			*error_out = "ffmpeg decode target is invalid";
		}
		return false;
	}
	if (!engine::bridge::ffmpeg::IsAvailable()) {
		if (error_out != nullptr) {
			*error_out = "ffmpeg bridge is unavailable";
		}
		return false;
	}

	engine::bridge::ffmpeg::MediaInfo media_info;
	const bool have_media_info = engine::bridge::ffmpeg::ProbeMedia(path.string(), &media_info, nullptr);

	std::vector<engine::bridge::ffmpeg::AudioFrameInfo> frames;
	if (!engine::bridge::ffmpeg::DecodeAudioFrames(path.string(), -1, 0, &frames, error_out)) {
		return false;
	}
	if (frames.empty()) {
		if (error_out != nullptr) {
			*error_out = "ffmpeg did not decode any audio frames";
		}
		return false;
	}

	decoded_audio->samples.clear();
	decoded_audio->sample_rate = 0;
	for (const auto& frame : frames) {
		if (frame.sample_rate <= 0) {
			if (error_out != nullptr) {
				*error_out = "decoded audio frame is missing sample rate";
			}
			return false;
		}
		if (decoded_audio->sample_rate == 0) {
			decoded_audio->sample_rate = frame.sample_rate;
		} else if (decoded_audio->sample_rate != frame.sample_rate) {
			if (error_out != nullptr) {
				*error_out = "decoded audio frames use inconsistent sample rates";
			}
			return false;
		}
		if (!AppendFrameAsMono(frame, &decoded_audio->samples, error_out)) {
			return false;
		}
	}

	decoded_audio->source_format = have_media_info && !media_info.format_name.empty()
		? media_info.format_name
		: "ffmpeg";
	decoded_audio->codec_name.clear();
	if (have_media_info) {
		for (const auto& stream : media_info.streams) {
			if (stream.media_type == "audio") {
				decoded_audio->codec_name = stream.codec_name;
				break;
			}
		}
	}
	decoded_audio->decode_backend = "ffmpeg";
	return !decoded_audio->samples.empty();
}

bool LoadAudioInput(
	const AudioDSPDemoOptions& options,
	DecodedAudio* decoded_audio,
	std::string* error_out) {
	if (decoded_audio == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio decode target is null";
		}
		return false;
	}

	const std::string extension = ToLowerCopy(options.input_path.extension().string());
	if (extension == ".raw" || extension == ".pcm") {
		return DecodeRawPcmFile(options.input_path, options.raw_sample_rate, decoded_audio, error_out);
	}

	std::string ffmpeg_error;
	if (engine::bridge::ffmpeg::IsAvailable()) {
		if (DecodeWithFFmpeg(options.input_path, decoded_audio, &ffmpeg_error)) {
			return true;
		}
	}

	if (extension == ".wav" && DecodeWavPcmFile(options.input_path, decoded_audio, error_out)) {
		return true;
	}

	if (error_out != nullptr) {
		if (!ffmpeg_error.empty()) {
			*error_out = ffmpeg_error;
		} else {
			*error_out = "no decoder available for input audio";
		}
	}
	return false;
}

bool ResolveProcessorKind(
	const std::string& processor_name,
	DemoProcessorKind* processor_kind,
	std::string* error_out) {
	if (processor_kind == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio processor target is null";
		}
		return false;
	}

	const std::string normalized = ToLowerCopy(processor_name);
	if (normalized == "spectral_shaper") {
		*processor_kind = DemoProcessorKind::kSpectralShaper;
		return true;
	}
	if (normalized == "phase_vocoder") {
		*processor_kind = DemoProcessorKind::kPhaseVocoder;
		return true;
	}

	if (error_out != nullptr) {
		*error_out = "unsupported audio processor: " + processor_name;
	}
	return false;
}

bool BuildSpectralShaperCurve(
	const std::string& profile_name,
	size_t positive_bins,
	std::vector<float>* curve_out,
	std::string* error_out) {
	if (curve_out == nullptr || positive_bins == 0) {
		if (error_out != nullptr) {
			*error_out = "spectral shaper curve target is invalid";
		}
		return false;
	}

	const std::string profile = ToLowerCopy(profile_name.empty() ? std::string("tilt") : profile_name);
	curve_out->assign(positive_bins, 1.0f);
	const float denominator = positive_bins > 1 ? static_cast<float>(positive_bins - 1) : 1.0f;

	if (profile == "unity") {
		return true;
	}
	if (profile == "tilt") {
		for (size_t bin = 0; bin < positive_bins; ++bin) {
			const float t = static_cast<float>(bin) / denominator;
			(*curve_out)[bin] = std::max(0.35f, 1.10f - 0.70f * t);
		}
		return true;
	}
	if (profile == "bright") {
		for (size_t bin = 0; bin < positive_bins; ++bin) {
			const float t = static_cast<float>(bin) / denominator;
			(*curve_out)[bin] = 0.75f + 0.60f * t;
		}
		return true;
	}

	if (error_out != nullptr) {
		*error_out = "unsupported spectral shaper profile: " + profile_name;
	}
	return false;
}

bool RunSelectedProcessorDemo(
	const AudioDSPDemoOptions& options,
	const std::vector<float>& input,
	int sample_rate,
	const std::string& mode,
	const std::string& source_format,
	const std::string& codec_name,
	const std::string& decode_backend,
	std::string* report_out,
	std::string* error_out) {
	if (report_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio demo report target is null";
		}
		return false;
	}
	if (input.empty() || sample_rate <= 0) {
		if (error_out != nullptr) {
			*error_out = "input audio buffer is empty";
		}
		return false;
	}

	DemoProcessorKind processor_kind = DemoProcessorKind::kSpectralShaper;
	if (!ResolveProcessorKind(options.processor.empty() ? std::string("spectral_shaper") : options.processor, &processor_kind, error_out)) {
		return false;
	}

	std::error_code fs_error;
	std::filesystem::create_directories(options.output_dir, fs_error);
	if (fs_error) {
		if (error_out != nullptr) {
			*error_out = "failed to create audio demo output directory";
		}
		return false;
	}

	const size_t latency_samples = kFFTSize - kHopSize;
	std::string processing_error;
	std::vector<float> processed_signal;
	std::string processor_label;
	std::string processor_detail;

	if (processor_kind == DemoProcessorKind::kSpectralShaper) {
		processor_label = "spectral_shaper";
		processor_detail = options.spectral_shaper_profile.empty() ? "tilt" : options.spectral_shaper_profile;

		Engine::Audio::DSP::SpectralShaper shaper(kFFTSize, kHopSize);
		std::vector<float> curve;
		if (!BuildSpectralShaperCurve(processor_detail, (kFFTSize / 2u) + 1u, &curve, error_out)) {
			return false;
		}
		if (!shaper.SetCurve(curve)) {
			if (error_out != nullptr) {
				*error_out = "failed to set spectral shaper curve";
			}
			return false;
		}
		processed_signal = ProcessSignal(&shaper, input, kHopSize, latency_samples, &processing_error);
	} else {
		processor_label = "phase_vocoder";
		processor_detail = absl::StrFormat("%0.3f", options.phase_vocoder_ratio);

		Engine::Audio::DSP::PhaseVocoder vocoder(kFFTSize, kHopSize);
		if (!vocoder.Initialize(static_cast<float>(sample_rate))) {
			if (error_out != nullptr) {
				*error_out = "failed to initialize phase vocoder";
			}
			return false;
		}
		vocoder.SetTimeStretchRatio(options.phase_vocoder_ratio);
		processed_signal = ProcessSignal(&vocoder, input, kHopSize, latency_samples, &processing_error);
	}

	if (processed_signal.empty()) {
		if (error_out != nullptr) {
			*error_out = processing_error.empty() ? "audio dsp processor failed" : processing_error;
		}
		return false;
	}

	const std::vector<float> aligned_output = TrimLatency(processed_signal, latency_samples, input.size());
	if (aligned_output.empty()) {
		if (error_out != nullptr) {
			*error_out = "failed to trim processed audio latency";
		}
		return false;
	}

	const std::filesystem::path input_wav = options.output_dir / "decoded_input.wav";
	const std::filesystem::path processed_wav = options.output_dir / (processor_label + std::string(".wav"));
	if (!EncodeWave(input_wav, input, sample_rate, error_out)
		|| !EncodeWave(processed_wav, aligned_output, sample_rate, error_out)) {
		return false;
	}

	*report_out = absl::StrFormat(
		"Audio DSP demo\n"
		"mode=%s\n"
		"input_path=%s\n"
		"source_format=%s\n"
		"codec=%s\n"
		"decode_backend=%s\n"
		"channel_mix=mono\n"
		"sample_rate=%d\n"
		"input_frames=%zu\n"
		"processed_frames=%zu\n"
		"fft_size=%zu\n"
		"hop_size=%zu\n"
		"latency_samples=%zu\n"
		"processor=%s\n"
		"processor_detail=%s\n"
		"input_wav=%s\n"
		"processed_wav=%s\n",
		mode,
		options.input_path.empty() ? "<synthetic>" : options.input_path.string(),
		source_format.empty() ? "unknown" : source_format,
		codec_name.empty() ? "unknown" : codec_name,
		decode_backend.empty() ? "generated" : decode_backend,
		sample_rate,
		input.size(),
		aligned_output.size(),
		kFFTSize,
		kHopSize,
		latency_samples,
		processor_label,
		processor_detail,
		input_wav.string(),
		processed_wav.string());

	if (error_out != nullptr) {
		error_out->clear();
	}
	return true;
}

bool RunSyntheticSmoke(
	const std::filesystem::path& output_dir,
	std::string* report_out,
	std::string* error_out) {
	if (report_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio demo report target is null";
		}
		return false;
	}

	std::error_code fs_error;
	std::filesystem::create_directories(output_dir, fs_error);
	if (fs_error) {
		if (error_out != nullptr) {
			*error_out = "failed to create audio demo output directory";
		}
		return false;
	}

	const std::vector<float> input = GenerateInputSignal(kSyntheticSignalSamples, kDefaultSampleRate);

	Engine::Audio::DSP::SpectralShaper shaper(kFFTSize, kHopSize);
	if (!shaper.SetCurve(std::vector<float>((kFFTSize / 2u) + 1u, 1.0f))) {
		if (error_out != nullptr) {
			*error_out = "failed to set spectral shaper curve";
		}
		return false;
	}

	Engine::Audio::DSP::PhaseVocoder vocoder(kFFTSize, kHopSize);
	if (!vocoder.Initialize(static_cast<float>(kDefaultSampleRate))) {
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
	const std::vector<float> shaper_aligned = TrimLatency(shaper_output, latency_samples, input.size());
	const std::vector<float> vocoder_aligned = TrimLatency(vocoder_output, latency_samples, input.size());
	if (shaper_aligned.empty() || vocoder_aligned.empty()) {
		if (error_out != nullptr) {
			*error_out = "failed to trim synthetic smoke outputs";
		}
		return false;
	}
	if (!EncodeWave(input_path, input, kDefaultSampleRate, error_out)
		|| !EncodeWave(shaper_path, shaper_aligned, kDefaultSampleRate, error_out)
		|| !EncodeWave(vocoder_path, vocoder_aligned, kDefaultSampleRate, error_out)) {
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
		"input_mode=synthetic\n"
		"input_wav=%s\n"
		"spectral_shaper_wav=%s\n"
		"phase_vocoder_wav=%s\n",
		smoke_ok ? "pass" : "fail",
		kDefaultSampleRate,
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

}  // namespace

bool RunAudioDSPDemo(
	const AudioDSPDemoOptions& options,
	std::string* report_out,
	std::string* error_out) {
	const std::string processor = ToLowerCopy(options.processor);
	if (options.input_path.empty() && (processor.empty() || processor == "smoke")) {
		return RunSyntheticSmoke(options.output_dir, report_out, error_out);
	}

	if (options.input_path.empty()) {
		const std::vector<float> synthetic_input = GenerateInputSignal(kSyntheticSignalSamples, kDefaultSampleRate);
		return RunSelectedProcessorDemo(
			options,
			synthetic_input,
			kDefaultSampleRate,
			"synthetic",
			"synthetic_tone_mix",
			"generated",
			"generated",
			report_out,
			error_out);
	}

	DecodedAudio decoded_audio;
	if (!LoadAudioInput(options, &decoded_audio, error_out)) {
		return false;
	}

	AudioDSPDemoOptions normalized_options = options;
	if (normalized_options.processor.empty()) {
		normalized_options.processor = "spectral_shaper";
	}
	return RunSelectedProcessorDemo(
		normalized_options,
		decoded_audio.samples,
		decoded_audio.sample_rate,
		"file",
		decoded_audio.source_format,
		decoded_audio.codec_name,
		decoded_audio.decode_backend,
		report_out,
		error_out);
}

bool RunAudioDSPDemo(
	const std::filesystem::path& output_dir,
	std::string* report_out,
	std::string* error_out) {
	AudioDSPDemoOptions options;
	options.output_dir = output_dir;
	return RunAudioDSPDemo(options, report_out, error_out);
}

}  // namespace Engine::Audio::Demo