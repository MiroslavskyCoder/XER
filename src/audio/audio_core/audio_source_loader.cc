#include "audio_source_loader.h"

#include "audio/audio_core/audio_interleave_processor.h"
#include "audio/file_io_codecs/codec_wav_pcm.h"
#include "wrapper/ffmpeg/ffmpeg_engine_bridge.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

namespace Engine::Audio::Core {

namespace {

struct DecodedAudioData {
	std::vector<std::vector<float>> channels;
	int sample_rate = 0;
	std::string source_format;
	std::string codec_name;
	std::string decode_backend;

	size_t FrameCount() const {
		return channels.empty() ? 0 : channels.front().size();
	}
};

std::string ToLowerCopy(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return value;
}

bool ReadBinaryFile(
	const std::filesystem::path& path,
	std::vector<std::uint8_t>* bytes_out,
	std::string* error_out) {
	if (bytes_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio byte target is null";
		}
		return false;
	}

	std::ifstream input(path, std::ios::binary | std::ios::ate);
	if (!input.is_open()) {
		if (error_out != nullptr) {
			*error_out = "failed to open audio source: " + path.string();
		}
		return false;
	}

	const std::streamsize size = input.tellg();
	if (size < 0) {
		if (error_out != nullptr) {
			*error_out = "failed to size audio source: " + path.string();
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
			*error_out = "failed to read audio source: " + path.string();
		}
		return false;
	}
	return true;
}

std::string CanonicalSampleFormat(std::string sample_format) {
	sample_format = ToLowerCopy(std::move(sample_format));
	if (!sample_format.empty() && sample_format.back() == 'p') {
		sample_format.pop_back();
	}
	return sample_format;
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

bool AppendFrameChannels(
	const engine::bridge::ffmpeg::AudioFrameInfo& frame,
	DecodedAudioData* decoded_audio,
	std::string* error_out) {
	if (decoded_audio == nullptr || frame.channels <= 0 || frame.sample_count <= 0) {
		if (error_out != nullptr) {
			*error_out = "invalid decoded audio frame";
		}
		return false;
	}

	if (decoded_audio->channels.empty()) {
		decoded_audio->channels.resize(static_cast<size_t>(frame.channels));
	} else if (decoded_audio->channels.size() != static_cast<size_t>(frame.channels)) {
		if (error_out != nullptr) {
			*error_out = "decoded audio frame changed channel count";
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

	for (auto& channel : decoded_audio->channels) {
		channel.reserve(channel.size() + static_cast<size_t>(frame.sample_count));
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

		for (int channel = 0; channel < frame.channels; ++channel) {
			for (int sample_index = 0; sample_index < frame.sample_count; ++sample_index) {
				float decoded_sample = 0.0f;
				const size_t byte_offset = plane_offsets[static_cast<size_t>(channel)]
					+ static_cast<size_t>(sample_index) * bytes_per_sample;
				if (!DecodeSample(frame.sample_format, frame.data.data() + static_cast<std::ptrdiff_t>(byte_offset), &decoded_sample)) {
					if (error_out != nullptr) {
						*error_out = "failed to decode planar audio sample";
					}
					return false;
				}
				decoded_audio->channels[static_cast<size_t>(channel)].push_back(decoded_sample);
			}
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

	for (int sample_index = 0; sample_index < frame.sample_count; ++sample_index) {
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
			decoded_audio->channels[static_cast<size_t>(channel)].push_back(decoded_sample);
		}
	}
	return true;
}

bool DecodeRawPcmFile(
	const std::filesystem::path& path,
	int sample_rate,
	DecodedAudioData* decoded_audio,
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

	decoded_audio->channels.assign(1, std::vector<float>(bytes.size() / 2u, 0.0f));
	for (size_t index = 0; index < decoded_audio->channels.front().size(); ++index) {
		const std::int16_t sample = static_cast<std::int16_t>(
			static_cast<std::uint16_t>(bytes[index * 2u])
			| (static_cast<std::uint16_t>(bytes[index * 2u + 1u]) << 8u));
		decoded_audio->channels.front()[index] = static_cast<float>(sample) / 32768.0f;
	}
	decoded_audio->sample_rate = sample_rate;
	decoded_audio->source_format = "raw_s16le_mono";
	decoded_audio->codec_name = "pcm_s16le";
	decoded_audio->decode_backend = "manual";
	return true;
}

bool DecodeWavPcmFile(
	const std::filesystem::path& path,
	DecodedAudioData* decoded_audio,
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
	decoded_audio->channels.assign(1, {});
	if (!codec.Decode16(bytes.data(), bytes.size(), decoded_audio->channels.front(), &sample_rate)) {
		if (error_out != nullptr) {
			*error_out = "failed to decode PCM WAV input";
		}
		return false;
	}

	decoded_audio->sample_rate = sample_rate;
	decoded_audio->source_format = "wav";
	decoded_audio->codec_name = "pcm_s16le";
	decoded_audio->decode_backend = "local_wav";
	return !decoded_audio->channels.front().empty();
}

bool DecodeWithFFmpeg(
	const std::filesystem::path& path,
	DecodedAudioData* decoded_audio,
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

	decoded_audio->channels.clear();
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
		if (!AppendFrameChannels(frame, decoded_audio, error_out)) {
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
	return decoded_audio->FrameCount() > 0;
}

bool MixToMono(
	const std::vector<std::vector<float>>& input_channels,
	std::vector<float>* mono_output,
	std::string* error_out) {
	if (mono_output == nullptr || input_channels.empty()) {
		if (error_out != nullptr) {
			*error_out = "audio channel mix target is invalid";
		}
		return false;
	}

	const size_t frame_count = input_channels.front().size();
	for (const auto& channel : input_channels) {
		if (channel.size() != frame_count) {
			if (error_out != nullptr) {
				*error_out = "decoded audio channels are not aligned";
			}
			return false;
		}
	}

	if (input_channels.size() == 1u) {
		*mono_output = input_channels.front();
		return true;
	}

	mono_output->assign(frame_count, 0.0f);
	std::vector<const float*> channel_ptrs(input_channels.size(), nullptr);
	for (size_t index = 0; index < input_channels.size(); ++index) {
		channel_ptrs[index] = input_channels[index].data();
	}

	if (!AudioInterleaveProcessor::MixChannels(
			channel_ptrs.data(),
			static_cast<int>(input_channels.size()),
			mono_output->data(),
			frame_count)) {
		if (error_out != nullptr) {
			*error_out = "failed to mix decoded audio channels";
		}
		return false;
	}
	return true;
}

bool ResampleMono(
	const std::vector<float>& input,
	int input_sample_rate,
	int output_sample_rate,
	ResampleQuality quality,
	std::vector<float>* output,
	std::string* error_out) {
	if (output == nullptr || input_sample_rate <= 0 || output_sample_rate <= 0) {
		if (error_out != nullptr) {
			*error_out = "audio resample configuration is invalid";
		}
		return false;
	}

	if (input.empty()) {
		output->clear();
		return true;
	}

	if (input_sample_rate == output_sample_rate) {
		*output = input;
		return true;
	}

	AudioSampleRateConverter converter(quality);
	if (!converter.Initialize(input_sample_rate, output_sample_rate)) {
		if (error_out != nullptr) {
			*error_out = "failed to initialize audio sample-rate converter";
		}
		return false;
	}

	const size_t predicted_frames = std::max<size_t>(
		1u,
		static_cast<size_t>(std::ceil(static_cast<double>(input.size()) * converter.GetRatio())));
	output->assign(predicted_frames, 0.0f);

	size_t output_frames = 0;
	if (!converter.Convert(input.data(), input.size(), output->data(), output_frames)) {
		if (error_out != nullptr) {
			*error_out = "failed to resample normalized audio";
		}
		return false;
	}
	output->resize(output_frames);
	return !output->empty();
}

}  // namespace

bool AudioSourceLoader::Load(
	const AudioSourceLoadOptions& options,
	AudioSourceBuffer* output,
	std::string* error_out) {
	if (output == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio source output target is null";
		}
		return false;
	}
	if (options.input_path.empty()) {
		if (error_out != nullptr) {
			*error_out = "audio input path is empty";
		}
		return false;
	}
	if (!std::filesystem::exists(options.input_path)) {
		if (error_out != nullptr) {
			*error_out = "audio input not found: " + options.input_path.string();
		}
		return false;
	}
	if (options.target_channels != 1) {
		if (error_out != nullptr) {
			*error_out = "only mono normalized audio is currently supported";
		}
		return false;
	}
	if (options.target_sample_rate <= 0 || options.raw_sample_rate <= 0) {
		if (error_out != nullptr) {
			*error_out = "audio normalization sample rate must be positive";
		}
		return false;
	}

	DecodedAudioData decoded_audio;
	const std::string extension = ToLowerCopy(options.input_path.extension().string());
	if (extension == ".raw" || extension == ".pcm") {
		if (!DecodeRawPcmFile(options.input_path, options.raw_sample_rate, &decoded_audio, error_out)) {
			return false;
		}
	} else {
		std::string ffmpeg_error;
		bool decoded = false;
		if (engine::bridge::ffmpeg::IsAvailable()) {
			decoded = DecodeWithFFmpeg(options.input_path, &decoded_audio, &ffmpeg_error);
		}
		if (!decoded && extension == ".wav") {
			decoded = DecodeWavPcmFile(options.input_path, &decoded_audio, error_out);
		}
		if (!decoded) {
			if (error_out != nullptr) {
				*error_out = !ffmpeg_error.empty() ? ffmpeg_error : "no decoder available for input audio";
			}
			return false;
		}
	}

	std::vector<float> mono_audio;
	if (!MixToMono(decoded_audio.channels, &mono_audio, error_out)) {
		return false;
	}

	std::vector<float> normalized_audio;
	if (!ResampleMono(
			mono_audio,
			decoded_audio.sample_rate,
			options.target_sample_rate,
			options.resample_quality,
			&normalized_audio,
			error_out)) {
		return false;
	}

	output->samples = std::move(normalized_audio);
	output->sample_rate = options.target_sample_rate;
	output->channels = options.target_channels;
	output->original_sample_rate = decoded_audio.sample_rate;
	output->original_channels = static_cast<int>(decoded_audio.channels.size());
	output->frame_count = output->samples.size();
	output->original_frame_count = decoded_audio.FrameCount();
	output->source_format = decoded_audio.source_format;
	output->codec_name = decoded_audio.codec_name;
	output->decode_backend = decoded_audio.decode_backend;
	return !output->samples.empty();
}

}  // namespace Engine::Audio::Core