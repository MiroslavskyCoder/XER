#include "modules/module_builders.h"

#include "audio/audio_core/audio_source_loader.h"
#include "audio/fx_customs/fx_customs_presets.h"
#include "audio/file_io_codecs/codec_wav_pcm.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace modules::detail {
namespace {

using Engine::Audio::Core::AudioSourceBuffer;
using Engine::Audio::Core::AudioSourceLoadOptions;
using Engine::Audio::Core::ResampleQuality;

struct AudioSignalStats {
	double rms = 0.0;
	float peak = 0.0f;
};

struct AudioBufferData {
	std::vector<float> samples;
	int sample_rate = 0;
	int channels = 0;
};

enum class AudioFxBatchMode {
	kParallel,
	kChain,
};

struct AudioBatchArtifact {
	std::string effect_name;
	std::string effect_slug;
	std::string render_mode;
	std::string stage_input_wav;
	std::string processed_wav;
	std::string report_path;
	std::string report_text;
	size_t stage_index = 0;
	size_t stage_input_frame_count = 0;
	size_t frame_count = 0;
	AudioSourceBuffer output_audio;
	Engine::Audio::FX::CustomEffectReport report;
};

struct AudioBatchOptions {
	AudioFxBatchMode batch_mode = AudioFxBatchMode::kParallel;
	std::string clap_plugin_reference = "builtin://gain";
	std::string output_dir;
};

struct AudioBatchResult {
	AudioFxBatchMode batch_mode = AudioFxBatchMode::kParallel;
	std::string output_dir;
	std::string normalized_input_wav;
	std::string final_output_wav;
	AudioSourceBuffer input_audio;
	AudioSourceBuffer final_audio;
	std::vector<AudioBatchArtifact> artifacts;
};

std::string NormalizeEffectName(std::string text) {
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	text.erase(std::remove(text.begin(), text.end(), ' '), text.end());
	return text;
}

std::string SlugifyEffectName(std::string text) {
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
		if (std::isalnum(ch) != 0) {
			return static_cast<char>(std::tolower(ch));
		}
		return static_cast<char>('_');
	});
	text.erase(std::unique(text.begin(), text.end(), [](char lhs, char rhs) {
		return lhs == '_' && rhs == '_';
	}), text.end());
	while (!text.empty() && text.front() == '_') {
		text.erase(text.begin());
	}
	while (!text.empty() && text.back() == '_') {
		text.pop_back();
	}
	return text.empty() ? std::string("fx_custom") : text;
}

std::string BuildStageSlug(size_t stage_index, const std::string& effect_name) {
	std::ostringstream output;
	output << std::setw(2) << std::setfill('0') << stage_index << "_" << SlugifyEffectName(effect_name);
	return output.str();
}

const char* AudioFxBatchModeName(AudioFxBatchMode batch_mode) {
	switch (batch_mode) {
	case AudioFxBatchMode::kParallel:
		return "parallel";
	case AudioFxBatchMode::kChain:
		return "chain";
	}
	return "parallel";
}

bool ParseAudioFxBatchMode(const std::string& text,
			   AudioFxBatchMode* batch_mode_out,
			   std::string* error_out) {
	if (batch_mode_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio fx batch mode target is null";
		}
		return false;
	}
	const std::string normalized = NormalizeEffectName(text.empty() ? std::string("parallel") : text);
	if (normalized == "parallel") {
		*batch_mode_out = AudioFxBatchMode::kParallel;
		return true;
	}
	if (normalized == "chain") {
		*batch_mode_out = AudioFxBatchMode::kChain;
		return true;
	}
	if (error_out != nullptr) {
		*error_out = "unsupported audio fx batch mode: " + text;
	}
	return false;
}

bool ValidateEffectNames(const std::vector<std::string>& effect_names,
			 std::string* error_out) {
	if (effect_names.empty()) {
		if (error_out != nullptr) {
			*error_out = "audio effect list requires at least one effect name";
		}
		return false;
	}
	for (const auto& effect_name : effect_names) {
		if (!Engine::Audio::FX::Customs::IsFxCustomPresetSupported(effect_name)) {
			if (error_out != nullptr) {
				*error_out = "unsupported custom effect preset: " + effect_name;
			}
			return false;
		}
	}
	return true;
}

bool RequireStringArrayArg(const v8::FunctionCallbackInfo<v8::Value>& args,
			   int index,
			   const char* name,
			   std::vector<std::string>* out) {
	if (out == nullptr) {
		Engine::Helper::ThrowError(args.GetIsolate(), "string array output target is null");
		return false;
	}
	out->clear();
	if (args.Length() <= index || !args[index]->IsArray()) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), name);
		return false;
	}
	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	v8::Local<v8::Array> array = args[index].As<v8::Array>();
	out->reserve(array->Length());
	for (uint32_t item_index = 0; item_index < array->Length(); ++item_index) {
		v8::Local<v8::Value> element;
		if (!array->Get(context, item_index).ToLocal(&element) || !element->IsString()) {
			Engine::Helper::ThrowTypeError(args.GetIsolate(), name);
			out->clear();
			return false;
		}
		out->push_back(Engine::Helper::FromV8Str(args.GetIsolate(), element));
	}
	return true;
}

int GetObjectInt(v8::Isolate* isolate,
			 v8::Local<v8::Context> context,
			 v8::Local<v8::Object> object,
			 const char* primary_key,
			 const char* secondary_key,
			 int fallback) {
	v8::Local<v8::Value> value;
	if (!GetObjectValue(isolate, context, object, primary_key, secondary_key, &value) || !value->IsNumber()) {
		return fallback;
	}
	return value->Int32Value(context).FromMaybe(fallback);
}

ResampleQuality ParseResampleQuality(std::string value) {
	value = ToLowerCopy(TrimWhitespace(std::move(value)));
	if (value == "fast") {
		return ResampleQuality::FAST;
	}
	if (value == "medium") {
		return ResampleQuality::MEDIUM;
	}
	if (value == "best") {
		return ResampleQuality::BEST;
	}
	return ResampleQuality::HIGH;
}

const char* ResampleQualityName(ResampleQuality quality) {
	switch (quality) {
	case ResampleQuality::FAST:
		return "fast";
	case ResampleQuality::MEDIUM:
		return "medium";
	case ResampleQuality::HIGH:
		return "high";
	case ResampleQuality::BEST:
		return "best";
	}
	return "high";
}

double SecondsFromFrames(size_t frame_count, int sample_rate) {
	if (sample_rate <= 0) {
		return 0.0;
	}
	return static_cast<double>(frame_count) / static_cast<double>(sample_rate);
}

AudioSignalStats ComputeAudioSignalStats(const std::vector<float>& samples) {
	AudioSignalStats stats;
	if (samples.empty()) {
		return stats;
	}

	double sum_squared = 0.0;
	for (float sample : samples) {
		const float absolute = std::fabs(sample);
		if (absolute > stats.peak) {
			stats.peak = absolute;
		}
		sum_squared += static_cast<double>(sample) * static_cast<double>(sample);
	}
	stats.rms = std::sqrt(sum_squared / static_cast<double>(samples.size()));
	return stats;
}

v8::Local<v8::Object> MakeAudioStatsObject(v8::Isolate* isolate,
				   v8::Local<v8::Context> context,
				   const AudioBufferData& audio_data) {
	const AudioSignalStats stats = ComputeAudioSignalStats(audio_data.samples);
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "peak", v8::Number::New(isolate, static_cast<double>(stats.peak)));
	SetProperty(isolate, context, object, "rms", v8::Number::New(isolate, stats.rms));
	SetProperty(isolate, context, object, "sampleCount", v8::Number::New(isolate, static_cast<double>(audio_data.samples.size())));
	SetProperty(isolate, context, object, "frameCount", v8::Number::New(isolate, static_cast<double>(audio_data.samples.size() / static_cast<size_t>(audio_data.channels))));
	SetProperty(isolate, context, object, "channels", v8::Integer::New(isolate, audio_data.channels));
	SetProperty(isolate, context, object, "sampleRate", v8::Integer::New(isolate, audio_data.sample_rate));
	SetProperty(isolate, context, object, "durationSeconds", v8::Number::New(isolate, SecondsFromFrames(audio_data.samples.size() / static_cast<size_t>(audio_data.channels), audio_data.sample_rate)));
	return object;
}

bool ReadSampleVector(v8::Local<v8::Context> context,
			      v8::Local<v8::Value> value,
			      std::vector<float>* out) {
	if (out == nullptr) {
		return false;
	}
	out->clear();

	if (value->IsFloat32Array()) {
		v8::Local<v8::Float32Array> array = value.As<v8::Float32Array>();
		out->resize(array->Length());
		if (!out->empty()) {
			array->CopyContents(out->data(), out->size() * sizeof(float));
		}
		return true;
	}

	if (value->IsFloat64Array()) {
		v8::Local<v8::Float64Array> array = value.As<v8::Float64Array>();
		std::vector<double> temp(array->Length(), 0.0);
		if (!temp.empty()) {
			array->CopyContents(temp.data(), temp.size() * sizeof(double));
		}
		out->reserve(temp.size());
		for (double sample : temp) {
			out->push_back(static_cast<float>(sample));
		}
		return true;
	}

	if (value->IsTypedArray()) {
		v8::Local<v8::TypedArray> array = value.As<v8::TypedArray>();
		v8::Local<v8::Object> object = array.As<v8::Object>();
		out->reserve(array->Length());
		for (uint32_t index = 0; index < array->Length(); ++index) {
			v8::Local<v8::Value> element;
			if (!object->Get(context, index).ToLocal(&element) || !element->IsNumber()) {
				return false;
			}
			out->push_back(static_cast<float>(element->NumberValue(context).FromMaybe(0.0)));
		}
		return true;
	}

	if (!value->IsArray()) {
		return false;
	}

	v8::Local<v8::Array> array = value.As<v8::Array>();
	out->reserve(array->Length());
	for (uint32_t index = 0; index < array->Length(); ++index) {
		v8::Local<v8::Value> element;
		if (!array->Get(context, index).ToLocal(&element) || !element->IsNumber()) {
			return false;
		}
		out->push_back(static_cast<float>(element->NumberValue(context).FromMaybe(0.0)));
	}
	return true;
}

bool ReadByteVector(v8::Local<v8::Context> context,
			    v8::Local<v8::Value> value,
			    std::vector<std::uint8_t>* out) {
	if (out == nullptr) {
		return false;
	}
	out->clear();

	if (value->IsUint8Array()) {
		v8::Local<v8::Uint8Array> array = value.As<v8::Uint8Array>();
		out->resize(array->Length());
		if (!out->empty()) {
			array->CopyContents(out->data(), out->size());
		}
		return true;
	}

	if (value->IsArrayBuffer()) {
		v8::Local<v8::ArrayBuffer> buffer = value.As<v8::ArrayBuffer>();
		const std::shared_ptr<v8::BackingStore> backing = buffer->GetBackingStore();
		if (!backing) {
			return false;
		}
		const auto* begin = static_cast<const std::uint8_t*>(backing->Data());
		out->assign(begin, begin + backing->ByteLength());
		return true;
	}

	if (!value->IsArray()) {
		return false;
	}

	v8::Local<v8::Array> array = value.As<v8::Array>();
	out->reserve(array->Length());
	for (uint32_t index = 0; index < array->Length(); ++index) {
		v8::Local<v8::Value> element;
		if (!array->Get(context, index).ToLocal(&element) || !element->IsNumber()) {
			return false;
		}
		const double numeric = element->NumberValue(context).FromMaybe(0.0);
		if (numeric < 0.0 || numeric > 255.0) {
			return false;
		}
		out->push_back(static_cast<std::uint8_t>(numeric));
	}
	return true;
}

v8::Local<v8::Array> MakeSampleArray(v8::Isolate* isolate,
			     v8::Local<v8::Context> context,
			     const std::vector<float>& samples) {
	v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(samples.size()));
	for (size_t index = 0; index < samples.size(); ++index) {
		array->Set(
			context,
			static_cast<uint32_t>(index),
			v8::Number::New(isolate, static_cast<double>(samples[index]))).FromMaybe(false);
	}
	return array;
}

v8::Local<v8::Array> MakeByteArray(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   const std::vector<std::uint8_t>& bytes) {
	v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(bytes.size()));
	for (size_t index = 0; index < bytes.size(); ++index) {
		array->Set(
			context,
			static_cast<uint32_t>(index),
			v8::Integer::New(isolate, static_cast<int>(bytes[index]))).FromMaybe(false);
	}
	return array;
}

v8::Local<v8::Object> MakeAudioBufferObject(v8::Isolate* isolate,
				    v8::Local<v8::Context> context,
				    const AudioSourceBuffer& buffer) {
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "samples", MakeSampleArray(isolate, context, buffer.samples));
	SetProperty(isolate, context, object, "sampleRate", v8::Integer::New(isolate, buffer.sample_rate));
	SetProperty(isolate, context, object, "channels", v8::Integer::New(isolate, buffer.channels));
	SetProperty(isolate, context, object, "frameCount", v8::Number::New(isolate, static_cast<double>(buffer.frame_count)));
	SetProperty(isolate, context, object, "durationSeconds", v8::Number::New(isolate, SecondsFromFrames(buffer.frame_count, buffer.sample_rate)));
	SetProperty(isolate, context, object, "originalSampleRate", v8::Integer::New(isolate, buffer.original_sample_rate));
	SetProperty(isolate, context, object, "originalChannels", v8::Integer::New(isolate, buffer.original_channels));
	SetProperty(isolate, context, object, "originalFrameCount", v8::Number::New(isolate, static_cast<double>(buffer.original_frame_count)));
	SetProperty(isolate, context, object, "originalDurationSeconds", v8::Number::New(isolate, SecondsFromFrames(buffer.original_frame_count, buffer.original_sample_rate)));
	SetProperty(isolate, context, object, "sourceFormat", Engine::Helper::ToV8Str(isolate, buffer.source_format));
	SetProperty(isolate, context, object, "codecName", Engine::Helper::ToV8Str(isolate, buffer.codec_name));
	SetProperty(isolate, context, object, "decodeBackend", Engine::Helper::ToV8Str(isolate, buffer.decode_backend));
	return object;
}

v8::Local<v8::Object> MakeCustomEffectReportObject(v8::Isolate* isolate,
				   v8::Local<v8::Context> context,
				   const Engine::Audio::FX::CustomEffectReport& report) {
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "label", Engine::Helper::ToV8Str(isolate, report.label));
	SetProperty(isolate, context, object, "nodeCount", v8::Number::New(isolate, static_cast<double>(report.node_count)));
	SetProperty(isolate, context, object, "stageCount", v8::Number::New(isolate, static_cast<double>(report.stage_count)));
	SetProperty(isolate, context, object, "channelCount", v8::Number::New(isolate, static_cast<double>(report.channel_count)));
	SetProperty(isolate, context, object, "workerCountUsed", v8::Number::New(isolate, static_cast<double>(report.worker_count_used)));
	SetProperty(isolate, context, object, "peak", v8::Number::New(isolate, static_cast<double>(report.peak)));
	SetProperty(isolate, context, object, "rms", v8::Number::New(isolate, report.rms));
	SetProperty(isolate, context, object, "stageReports", MakeStringArray(isolate, context, report.stage_reports));
	SetProperty(isolate, context, object, "nodeReports", MakeStringArray(isolate, context, report.node_reports));
	return object;
}

v8::Local<v8::Object> MakeAudioInspectObject(v8::Isolate* isolate,
				     v8::Local<v8::Context> context,
				     const std::string& input_path,
				     const AudioSourceBuffer& buffer,
				     const AudioSignalStats& stats,
				     const AudioSourceLoadOptions& options) {
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	v8::Local<v8::Object> source = v8::Object::New(isolate);
	v8::Local<v8::Object> normalized = v8::Object::New(isolate);
	v8::Local<v8::Object> option_values = v8::Object::New(isolate);

	SetProperty(isolate, context, object, "inputPath", Engine::Helper::ToV8Str(isolate, input_path));

	SetProperty(isolate, context, source, "format", Engine::Helper::ToV8Str(isolate, buffer.source_format.empty() ? "unknown" : buffer.source_format));
	SetProperty(isolate, context, source, "codec", Engine::Helper::ToV8Str(isolate, buffer.codec_name.empty() ? "unknown" : buffer.codec_name));
	SetProperty(isolate, context, source, "decodeBackend", Engine::Helper::ToV8Str(isolate, buffer.decode_backend.empty() ? "unknown" : buffer.decode_backend));
	SetProperty(isolate, context, source, "sampleRate", v8::Integer::New(isolate, buffer.original_sample_rate));
	SetProperty(isolate, context, source, "channels", v8::Integer::New(isolate, buffer.original_channels));
	SetProperty(isolate, context, source, "frames", v8::Number::New(isolate, static_cast<double>(buffer.original_frame_count)));
	SetProperty(isolate, context, source, "durationSeconds", v8::Number::New(isolate, SecondsFromFrames(buffer.original_frame_count, buffer.original_sample_rate)));

	SetProperty(isolate, context, normalized, "sampleRate", v8::Integer::New(isolate, buffer.sample_rate));
	SetProperty(isolate, context, normalized, "channels", v8::Integer::New(isolate, buffer.channels));
	SetProperty(isolate, context, normalized, "frames", v8::Number::New(isolate, static_cast<double>(buffer.frame_count)));
	SetProperty(isolate, context, normalized, "durationSeconds", v8::Number::New(isolate, SecondsFromFrames(buffer.frame_count, buffer.sample_rate)));
	SetProperty(isolate, context, normalized, "peak", v8::Number::New(isolate, static_cast<double>(stats.peak)));
	SetProperty(isolate, context, normalized, "rms", v8::Number::New(isolate, stats.rms));

	SetProperty(isolate, context, option_values, "rawSampleRate", v8::Integer::New(isolate, options.raw_sample_rate));
	SetProperty(isolate, context, option_values, "targetSampleRate", v8::Integer::New(isolate, options.target_sample_rate));
	SetProperty(isolate, context, option_values, "targetChannels", v8::Integer::New(isolate, options.target_channels));
	SetProperty(isolate, context, option_values, "resampleQuality", Engine::Helper::ToV8Str(isolate, ResampleQualityName(options.resample_quality)));

	SetProperty(isolate, context, object, "source", source);
	SetProperty(isolate, context, object, "normalized", normalized);
	SetProperty(isolate, context, object, "options", option_values);
	return object;
}

AudioSourceLoadOptions ParseLoadOptions(v8::Isolate* isolate,
				v8::Local<v8::Context> context,
				v8::Local<v8::Value> value) {
	AudioSourceLoadOptions options;
	options.raw_sample_rate = 44100;
	options.target_sample_rate = 44100;
	options.target_channels = 1;
	options.resample_quality = ResampleQuality::HIGH;
	if (!value->IsObject()) {
		return options;
	}
	v8::Local<v8::Object> object = value.As<v8::Object>();
	options.raw_sample_rate = GetObjectInt(isolate, context, object, "rawSampleRate", "raw_sample_rate", options.raw_sample_rate);
	options.target_sample_rate = GetObjectInt(isolate, context, object, "targetSampleRate", "target_sample_rate", options.target_sample_rate);
	options.target_channels = GetObjectInt(isolate, context, object, "targetChannels", "target_channels", options.target_channels);
	options.resample_quality = ParseResampleQuality(GetObjectString(isolate, context, object, "resampleQuality", "resample_quality", "high"));
	return options;
}

bool TryReadAudioBufferObject(v8::Isolate* isolate,
			      v8::Local<v8::Context> context,
			      v8::Local<v8::Value> value,
			      AudioBufferData* out) {
	if (out == nullptr || !value->IsObject()) {
		return false;
	}
	v8::Local<v8::Object> object = value.As<v8::Object>();
	v8::Local<v8::Value> samples_value;
	if (!GetObjectValue(isolate, context, object, "samples", nullptr, &samples_value)) {
		return false;
	}
	if (!ReadSampleVector(context, samples_value, &out->samples)) {
		return false;
	}
	out->sample_rate = GetObjectInt(isolate, context, object, "sampleRate", "sample_rate", 0);
	out->channels = GetObjectInt(isolate, context, object, "channels", nullptr, 0);
	return true;
}

bool ParseAudioBufferData(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  const v8::FunctionCallbackInfo<v8::Value>& args,
			  int value_index,
			  int options_index,
			  AudioBufferData* out) {
	if (out == nullptr) {
		Engine::Helper::ThrowError(isolate, "audio buffer target is null");
		return false;
	}
	if (args.Length() <= value_index) {
		Engine::Helper::ThrowTypeError(isolate, "audio buffer argument is required");
		return false;
	}

	AudioBufferData parsed;
	if (!TryReadAudioBufferObject(isolate, context, args[value_index], &parsed)
		&& !ReadSampleVector(context, args[value_index], &parsed.samples)) {
		Engine::Helper::ThrowTypeError(isolate, "audio value expects { samples, sampleRate, channels } or a numeric array/typed array");
		return false;
	}

	if (args.Length() > options_index && args[options_index]->IsObject()) {
		v8::Local<v8::Object> options = args[options_index].As<v8::Object>();
		parsed.sample_rate = GetObjectInt(isolate, context, options, "sampleRate", "sample_rate", parsed.sample_rate);
		parsed.channels = GetObjectInt(isolate, context, options, "channels", nullptr, parsed.channels);
	}

	if (parsed.sample_rate <= 0) {
		parsed.sample_rate = 44100;
	}
	if (parsed.channels <= 0) {
		parsed.channels = 1;
	}
	if (parsed.samples.empty()) {
		Engine::Helper::ThrowError(isolate, "audio buffer has no samples");
		return false;
	}
	if ((parsed.samples.size() % static_cast<size_t>(parsed.channels)) != 0u) {
		Engine::Helper::ThrowError(isolate, "audio sample count is not divisible by channel count");
		return false;
	}

	*out = std::move(parsed);
	return true;
}

AudioSourceBuffer BuildAudioSourceBuffer(const AudioBufferData& data,
				 const std::string& source_format,
				 const std::string& codec_name,
				 const std::string& decode_backend) {
	AudioSourceBuffer buffer;
	buffer.samples = data.samples;
	buffer.sample_rate = data.sample_rate;
	buffer.channels = data.channels;
	buffer.original_sample_rate = data.sample_rate;
	buffer.original_channels = data.channels;
	buffer.frame_count = buffer.samples.size() / static_cast<size_t>(data.channels);
	buffer.original_frame_count = buffer.frame_count;
	buffer.source_format = source_format;
	buffer.codec_name = codec_name;
	buffer.decode_backend = decode_backend;
	return buffer;
}

bool EncodeWavBytes(const AudioBufferData& audio_data,
		    std::vector<std::uint8_t>* encoded_out,
		    std::string* error_out) {
	if (encoded_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "wav byte output target is null";
		}
		return false;
	}
	Engine::Audio::CodecIO::WavPcmCodec codec;
	const size_t frame_count = audio_data.samples.size() / static_cast<size_t>(audio_data.channels);
	if (!codec.Encode16(audio_data.samples.data(), frame_count, *encoded_out, audio_data.sample_rate, audio_data.channels)) {
		if (error_out != nullptr) {
			*error_out = "failed to encode wav data";
		}
		return false;
	}
	return true;
}

bool WriteTextFile(const std::filesystem::path& path,
		   const std::string& text,
		   std::string* error_out) {
	const std::vector<std::uint8_t> bytes(text.begin(), text.end());
	return WriteAllBytes(path, bytes, false, error_out);
}

bool WriteWaveFile(const std::filesystem::path& path,
		   const AudioBufferData& audio,
		   std::string* error_out) {
	std::vector<std::uint8_t> encoded;
	if (!EncodeWavBytes(audio, &encoded, error_out)) {
		return false;
	}
	return WriteAllBytes(path, encoded, false, error_out);
}

bool WriteWaveFile(const std::filesystem::path& path,
		   const AudioSourceBuffer& audio,
		   std::string* error_out) {
	return WriteWaveFile(path, AudioBufferData{audio.samples, audio.sample_rate, audio.channels}, error_out);
}

bool RenderCustomPreset(const std::string& effect_name,
			const AudioBufferData& input_audio,
			const std::string& clap_plugin_reference,
			AudioSourceBuffer* output_buffer,
			Engine::Audio::FX::CustomEffectReport* report_out,
			std::string* error_out) {
	if (output_buffer == nullptr || report_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "custom effect output target is null";
		}
		return false;
	}
	std::vector<float> processed_samples;
	if (!Engine::Audio::FX::Customs::RenderFxCustomPresetInterleaved(
			effect_name,
			static_cast<float>(input_audio.sample_rate),
			input_audio.samples,
			input_audio.channels,
			&processed_samples,
			report_out,
			clap_plugin_reference,
			error_out)) {
		return false;
	}
	*output_buffer = BuildAudioSourceBuffer(
		AudioBufferData{std::move(processed_samples), input_audio.sample_rate, input_audio.channels},
		"fx_custom",
		effect_name,
		"fx_customs");
	return true;
}

v8::Local<v8::Object> MakeEffectResultObject(v8::Isolate* isolate,
				    v8::Local<v8::Context> context,
				    const std::string& effect_name,
				    const AudioSourceBuffer& buffer,
				    const Engine::Audio::FX::CustomEffectReport& report) {
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "effectName", Engine::Helper::ToV8Str(isolate, effect_name));
	SetProperty(isolate, context, object, "audio", MakeAudioBufferObject(isolate, context, buffer));
	SetProperty(isolate, context, object, "report", MakeCustomEffectReportObject(isolate, context, report));
	SetProperty(isolate, context, object, "reportText", Engine::Helper::ToV8Str(isolate, Engine::Audio::FX::BuildCustomEffectReportText(report)));
	return object;
}

v8::Local<v8::Object> MakeBatchArtifactObject(v8::Isolate* isolate,
				      v8::Local<v8::Context> context,
				      const AudioBatchArtifact& artifact) {
	v8::Local<v8::Object> object = MakeEffectResultObject(isolate, context, artifact.effect_name, artifact.output_audio, artifact.report);
	SetProperty(isolate, context, object, "effectSlug", Engine::Helper::ToV8Str(isolate, artifact.effect_slug));
	SetProperty(isolate, context, object, "renderMode", Engine::Helper::ToV8Str(isolate, artifact.render_mode));
	SetProperty(isolate, context, object, "stageIndex", v8::Number::New(isolate, static_cast<double>(artifact.stage_index)));
	SetProperty(isolate, context, object, "stageInputFrameCount", v8::Number::New(isolate, static_cast<double>(artifact.stage_input_frame_count)));
	SetProperty(isolate, context, object, "stageInputDurationSeconds", v8::Number::New(isolate, SecondsFromFrames(artifact.stage_input_frame_count, artifact.output_audio.sample_rate)));
	SetProperty(isolate, context, object, "frameCount", v8::Number::New(isolate, static_cast<double>(artifact.frame_count)));
	SetProperty(isolate, context, object, "stageInputWav", Engine::Helper::ToV8Str(isolate, artifact.stage_input_wav));
	SetProperty(isolate, context, object, "processedWav", Engine::Helper::ToV8Str(isolate, artifact.processed_wav));
	SetProperty(isolate, context, object, "reportPath", Engine::Helper::ToV8Str(isolate, artifact.report_path));
	return object;
}

v8::Local<v8::Object> MakeBatchResultObject(v8::Isolate* isolate,
				    v8::Local<v8::Context> context,
				    const std::vector<std::string>& effect_names,
				    const AudioBatchResult& batch_result) {
	v8::Local<v8::Array> stages = v8::Array::New(isolate, static_cast<int>(batch_result.artifacts.size()));
	for (size_t stage_index = 0; stage_index < batch_result.artifacts.size(); ++stage_index) {
		stages->Set(
			context,
			static_cast<uint32_t>(stage_index),
			MakeBatchArtifactObject(isolate, context, batch_result.artifacts[stage_index])).FromMaybe(false);
	}
	v8::Local<v8::Object> result = v8::Object::New(isolate);
	SetProperty(isolate, context, result, "batchMode", Engine::Helper::ToV8Str(isolate, AudioFxBatchModeName(batch_result.batch_mode)));
	SetProperty(isolate, context, result, "effectNames", MakeStringArray(isolate, context, effect_names));
	SetProperty(isolate, context, result, "stageCount", v8::Number::New(isolate, static_cast<double>(batch_result.artifacts.size())));
	SetProperty(isolate, context, result, "inputAudio", MakeAudioBufferObject(isolate, context, batch_result.input_audio));
	SetProperty(isolate, context, result, "normalizedInputWav", Engine::Helper::ToV8Str(isolate, batch_result.normalized_input_wav));
	SetProperty(isolate, context, result, "outputDir", Engine::Helper::ToV8Str(isolate, batch_result.output_dir));
	SetProperty(isolate, context, result, "finalOutputWav", Engine::Helper::ToV8Str(isolate, batch_result.final_output_wav));
	SetProperty(isolate, context, result, "stages", stages);
	SetProperty(isolate, context, result, "audio", MakeAudioBufferObject(isolate, context, batch_result.final_audio));
	return result;
}

v8::Local<v8::Object> MakeChainResultObject(v8::Isolate* isolate,
				    v8::Local<v8::Context> context,
				    const std::vector<std::string>& effect_names,
				    const AudioBatchResult& batch_result) {
	v8::Local<v8::Array> stages = v8::Array::New(isolate, static_cast<int>(batch_result.artifacts.size()));
	for (size_t stage_index = 0; stage_index < batch_result.artifacts.size(); ++stage_index) {
		const AudioBatchArtifact& artifact = batch_result.artifacts[stage_index];
		stages->Set(
			context,
			static_cast<uint32_t>(stage_index),
			MakeEffectResultObject(isolate, context, artifact.effect_name, artifact.output_audio, artifact.report)).FromMaybe(false);
	}
	v8::Local<v8::Object> result = v8::Object::New(isolate);
	SetProperty(isolate, context, result, "effectNames", MakeStringArray(isolate, context, effect_names));
	SetProperty(isolate, context, result, "stageCount", v8::Number::New(isolate, static_cast<double>(batch_result.artifacts.size())));
	SetProperty(isolate, context, result, "stages", stages);
	SetProperty(isolate, context, result, "audio", MakeAudioBufferObject(isolate, context, batch_result.final_audio));
	return result;
}

bool RunAudioBatchEngine(const std::vector<std::string>& effect_names,
			 const AudioBufferData& input_audio,
			 const AudioBatchOptions& options,
			 AudioBatchResult* result_out,
			 std::string* error_out) {
	if (result_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio batch result target is null";
		}
		return false;
	}
	if (!ValidateEffectNames(effect_names, error_out)) {
		return false;
	}

	AudioBatchResult result;
	result.batch_mode = options.batch_mode;
	result.output_dir = options.output_dir;
	result.input_audio = BuildAudioSourceBuffer(input_audio, "batch_input", "batch_input", "audio_module");
	result.final_audio = result.input_audio;

	std::filesystem::path output_dir_path;
	if (!options.output_dir.empty()) {
		output_dir_path = std::filesystem::path(options.output_dir);
		std::error_code fs_error;
		std::filesystem::create_directories(output_dir_path, fs_error);
		if (fs_error) {
			if (error_out != nullptr) {
				*error_out = "failed to create audio batch output directory";
			}
			return false;
		}
		result.normalized_input_wav = (output_dir_path / "normalized_input.wav").string();
		if (!WriteWaveFile(std::filesystem::path(result.normalized_input_wav), input_audio, error_out)) {
			return false;
		}
	}

	AudioBufferData chain_input = input_audio;
	std::string chain_input_wav = result.normalized_input_wav;
	for (size_t stage_index = 0; stage_index < effect_names.size(); ++stage_index) {
		const std::string& effect_name = effect_names[stage_index];
		const AudioBufferData& stage_input = options.batch_mode == AudioFxBatchMode::kChain ? chain_input : input_audio;
		AudioSourceBuffer stage_output;
		Engine::Audio::FX::CustomEffectReport report;
		if (!RenderCustomPreset(effect_name, stage_input, options.clap_plugin_reference, &stage_output, &report, error_out)) {
			return false;
		}

		AudioBatchArtifact artifact;
		artifact.effect_name = effect_name;
		artifact.effect_slug = effect_names.size() > 1u ? BuildStageSlug(stage_index, effect_name) : SlugifyEffectName(effect_name);
		artifact.render_mode = AudioFxBatchModeName(options.batch_mode);
		artifact.stage_index = stage_index;
		artifact.stage_input_frame_count = stage_input.samples.size() / static_cast<size_t>(stage_input.channels);
		artifact.frame_count = stage_output.frame_count;
		artifact.report = report;
		artifact.report_text = Engine::Audio::FX::BuildCustomEffectReportText(report);
		artifact.output_audio = stage_output;
		if (!options.output_dir.empty()) {
			artifact.stage_input_wav = options.batch_mode == AudioFxBatchMode::kChain ? chain_input_wav : result.normalized_input_wav;
			artifact.processed_wav = (output_dir_path / (artifact.effect_slug + ".wav")).string();
			artifact.report_path = (output_dir_path / (artifact.effect_slug + "_report.txt")).string();
			if (!WriteWaveFile(std::filesystem::path(artifact.processed_wav), artifact.output_audio, error_out)) {
				return false;
			}
			if (!WriteTextFile(std::filesystem::path(artifact.report_path), artifact.report_text, error_out)) {
				return false;
			}
			result.final_output_wav = artifact.processed_wav;
		}

		result.final_audio = artifact.output_audio;
		result.artifacts.push_back(std::move(artifact));
		if (options.batch_mode == AudioFxBatchMode::kChain) {
			chain_input = AudioBufferData{result.final_audio.samples, result.final_audio.sample_rate, result.final_audio.channels};
			if (!result.artifacts.back().processed_wav.empty()) {
				chain_input_wav = result.artifacts.back().processed_wav;
			}
		}
	}

	*result_out = std::move(result);
	return true;
}

void AudioLoadCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string input_path;
	if (!RequireStringArg(args, 0, "load expects path string", &input_path)) {
		return;
	}

	v8::Local<v8::Value> options_value = v8::Undefined(isolate);
	if (args.Length() > 1) {
		options_value = args[1];
	}
	AudioSourceLoadOptions options = ParseLoadOptions(
		isolate,
		context,
		options_value);
	options.input_path = std::filesystem::path(input_path);

	AudioSourceBuffer buffer;
	std::string error;
	if (!Engine::Audio::Core::AudioSourceLoader::Load(options, &buffer, &error)) {
		Engine::Helper::ThrowError(isolate, error.empty() ? "audio load failed" : error);
		return;
	}
	args.GetReturnValue().Set(MakeAudioBufferObject(isolate, context, buffer));
}

void AudioInspectCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string input_path;
	if (!RequireStringArg(args, 0, "inspect expects path string", &input_path)) {
		return;
	}

	v8::Local<v8::Value> options_value = v8::Undefined(isolate);
	if (args.Length() > 1) {
		options_value = args[1];
	}
	AudioSourceLoadOptions options = ParseLoadOptions(
		isolate,
		context,
		options_value);
	options.input_path = std::filesystem::path(input_path);

	AudioSourceBuffer buffer;
	std::string error;
	if (!Engine::Audio::Core::AudioSourceLoader::Load(options, &buffer, &error)) {
		Engine::Helper::ThrowError(isolate, error.empty() ? "audio inspect failed" : error);
		return;
	}
	const AudioSignalStats stats = ComputeAudioSignalStats(buffer.samples);
	args.GetReturnValue().Set(MakeAudioInspectObject(isolate, context, input_path, buffer, stats, options));
}

void AudioEncodeWavCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	AudioBufferData audio_data;
	if (!ParseAudioBufferData(isolate, context, args, 0, 1, &audio_data)) {
		return;
	}

	std::vector<std::uint8_t> encoded;
	std::string error;
	if (!EncodeWavBytes(audio_data, &encoded, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	args.GetReturnValue().Set(MakeByteArray(isolate, context, encoded));
}

void AudioDecodeWavCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	if (args.Length() < 1) {
		Engine::Helper::ThrowTypeError(isolate, "decodeWav expects wav bytes");
		return;
	}

	std::vector<std::uint8_t> bytes;
	if (!ReadByteVector(context, args[0], &bytes)) {
		Engine::Helper::ThrowTypeError(isolate, "decodeWav expects Uint8Array, ArrayBuffer, or number[]");
		return;
	}

	Engine::Audio::CodecIO::WavPcmCodec codec;
	std::vector<float> samples;
	int sample_rate = 0;
	int channels = 0;
	if (!codec.Decode16(bytes.data(), bytes.size(), samples, &sample_rate, &channels)) {
		Engine::Helper::ThrowError(isolate, "failed to decode wav data");
		return;
	}

	const AudioSourceBuffer buffer = BuildAudioSourceBuffer(
		AudioBufferData{std::move(samples), sample_rate, channels},
		"wav",
		"pcm_s16le",
		"wav_pcm");
	args.GetReturnValue().Set(MakeAudioBufferObject(isolate, context, buffer));
}

void AudioSaveWavCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string output_path;
	if (!RequireStringArg(args, 0, "saveWav expects output path string", &output_path)) {
		return;
	}

	AudioBufferData audio_data;
	if (!ParseAudioBufferData(isolate, context, args, 1, 2, &audio_data)) {
		return;
	}

	std::vector<std::uint8_t> encoded;
	const size_t frame_count = audio_data.samples.size() / static_cast<size_t>(audio_data.channels);
	std::string error;
	if (!EncodeWavBytes(audio_data, &encoded, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}

	if (!WriteAllBytes(std::filesystem::path(output_path), encoded, false, &error)) {
		Engine::Helper::ThrowError(isolate, error.empty() ? "failed to write wav output" : error);
		return;
	}

	v8::Local<v8::Object> result = v8::Object::New(isolate);
	SetProperty(isolate, context, result, "outputPath", Engine::Helper::ToV8Str(isolate, output_path));
	SetProperty(isolate, context, result, "bytesWritten", v8::Number::New(isolate, static_cast<double>(encoded.size())));
	SetProperty(isolate, context, result, "sampleRate", v8::Integer::New(isolate, audio_data.sample_rate));
	SetProperty(isolate, context, result, "channels", v8::Integer::New(isolate, audio_data.channels));
	SetProperty(isolate, context, result, "frameCount", v8::Number::New(isolate, static_cast<double>(frame_count)));
	SetProperty(isolate, context, result, "durationSeconds", v8::Number::New(isolate, SecondsFromFrames(frame_count, audio_data.sample_rate)));
	args.GetReturnValue().Set(result);
}

void AudioStatsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	AudioBufferData audio_data;
	if (!ParseAudioBufferData(isolate, context, args, 0, 1, &audio_data)) {
		return;
	}
	args.GetReturnValue().Set(MakeAudioStatsObject(isolate, context, audio_data));
}

void AudioAvailableEffectsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	args.GetReturnValue().Set(MakeStringArray(isolate, context, Engine::Audio::FX::Customs::ListFxCustomPresetNames()));
}

void AudioHasEffectCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string effect_name;
	if (!RequireStringArg(args, 0, "hasEffect expects effect name string", &effect_name)) {
		return;
	}
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), Engine::Audio::FX::Customs::IsFxCustomPresetSupported(effect_name)));
}

void AudioApplyEffectCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string effect_name;
	if (!RequireStringArg(args, 0, "applyEffect expects effect name string", &effect_name)) {
		return;
	}
	AudioBufferData input_audio;
	if (!ParseAudioBufferData(isolate, context, args, 1, 2, &input_audio)) {
		return;
	}
	AudioBatchOptions batch_options;
	if (args.Length() > 2 && args[2]->IsObject()) {
		batch_options.clap_plugin_reference = GetObjectString(
			isolate,
			context,
			args[2].As<v8::Object>(),
			"clapPluginReference",
			"clap_plugin_reference",
			batch_options.clap_plugin_reference);
	}
	AudioBatchResult batch_result;
	std::string error;
	if (!RunAudioBatchEngine({effect_name}, input_audio, batch_options, &batch_result, &error)) {
		Engine::Helper::ThrowError(isolate, error.empty() ? "failed to apply custom effect" : error);
		return;
	}
	const AudioBatchArtifact& artifact = batch_result.artifacts.front();
	args.GetReturnValue().Set(MakeEffectResultObject(isolate, context, artifact.effect_name, artifact.output_audio, artifact.report));
}

void AudioApplyChainCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::vector<std::string> effect_names;
	if (!RequireStringArrayArg(args, 0, "applyChain expects effect name array", &effect_names)) {
		return;
	}
	if (effect_names.empty()) {
		Engine::Helper::ThrowError(isolate, "applyChain requires at least one effect name");
		return;
	}
	AudioBufferData input_audio;
	if (!ParseAudioBufferData(isolate, context, args, 1, 2, &input_audio)) {
		return;
	}
	AudioBatchOptions batch_options;
	batch_options.batch_mode = AudioFxBatchMode::kChain;
	if (args.Length() > 2 && args[2]->IsObject()) {
		batch_options.clap_plugin_reference = GetObjectString(
			isolate,
			context,
			args[2].As<v8::Object>(),
			"clapPluginReference",
			"clap_plugin_reference",
			batch_options.clap_plugin_reference);
	}
	AudioBatchResult batch_result;
	std::string error;
	if (!RunAudioBatchEngine(effect_names, input_audio, batch_options, &batch_result, &error)) {
		Engine::Helper::ThrowError(isolate, error.empty() ? "failed to apply effect chain" : error);
		return;
	}
	args.GetReturnValue().Set(MakeChainResultObject(isolate, context, effect_names, batch_result));
}

void AudioApplyBatchCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::vector<std::string> effect_names;
	if (!RequireStringArrayArg(args, 0, "applyBatch expects effect name array", &effect_names)) {
		return;
	}
	if (effect_names.empty()) {
		Engine::Helper::ThrowError(isolate, "applyBatch requires at least one effect name");
		return;
	}
	AudioBufferData input_audio;
	if (!ParseAudioBufferData(isolate, context, args, 1, 2, &input_audio)) {
		return;
	}
	AudioBatchOptions batch_options;
	if (args.Length() > 2 && args[2]->IsObject()) {
		v8::Local<v8::Object> options = args[2].As<v8::Object>();
		std::string batch_mode_text = GetObjectString(isolate, context, options, "batchMode", "batch_mode", "parallel");
		std::string error;
		if (!ParseAudioFxBatchMode(batch_mode_text, &batch_options.batch_mode, &error)) {
			Engine::Helper::ThrowError(isolate, error);
			return;
		}
		batch_options.clap_plugin_reference = GetObjectString(
			isolate,
			context,
			options,
			"clapPluginReference",
			"clap_plugin_reference",
			batch_options.clap_plugin_reference);
		batch_options.output_dir = GetObjectString(isolate, context, options, "outputDir", "output_dir", std::string());
	}
	AudioBatchResult batch_result;
	std::string error;
	if (!RunAudioBatchEngine(effect_names, input_audio, batch_options, &batch_result, &error)) {
		Engine::Helper::ThrowError(isolate, error.empty() ? "failed to apply effect batch" : error);
		return;
	}
	args.GetReturnValue().Set(MakeBatchResultObject(isolate, context, effect_names, batch_result));
}

}  // namespace

bool BuildAudioModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  v8::Local<v8::Object>* module_out,
			  std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && SetProperty(isolate, context, module, "defaultRawSampleRate", v8::Integer::New(isolate, 44100));
	ok = ok && SetProperty(isolate, context, module, "defaultTargetSampleRate", v8::Integer::New(isolate, 44100));
	ok = ok && SetProperty(isolate, context, module, "defaultTargetChannels", v8::Integer::New(isolate, 1));
	ok = ok && SetProperty(
		isolate,
		context,
		module,
		"resampleQualities",
		MakeStringArray(isolate, context, {"fast", "medium", "high", "best"}));
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "stats", &AudioStatsCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "availableEffects", &AudioAvailableEffectsCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "hasEffect", &AudioHasEffectCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "applyEffect", &AudioApplyEffectCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "applyBatch", &AudioApplyBatchCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "applyChain", &AudioApplyChainCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "load", &AudioLoadCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "inspect", &AudioInspectCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "save", &AudioSaveWavCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "saveWav", &AudioSaveWavCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "encodeWav", &AudioEncodeWavCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "decodeWav", &AudioDecodeWavCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Audio module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail
