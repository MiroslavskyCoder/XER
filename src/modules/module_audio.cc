#include "modules/module_builders.h"

#include "audio/audio_core/audio_source_loader.h"
#include "audio/file_io_codecs/codec_wav_pcm.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
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

	Engine::Audio::CodecIO::WavPcmCodec codec;
	std::vector<std::uint8_t> encoded;
	const size_t frame_count = audio_data.samples.size() / static_cast<size_t>(audio_data.channels);
	if (!codec.Encode16(audio_data.samples.data(), frame_count, encoded, audio_data.sample_rate, audio_data.channels)) {
		Engine::Helper::ThrowError(isolate, "failed to encode wav data");
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

	Engine::Audio::CodecIO::WavPcmCodec codec;
	std::vector<std::uint8_t> encoded;
	const size_t frame_count = audio_data.samples.size() / static_cast<size_t>(audio_data.channels);
	if (!codec.Encode16(audio_data.samples.data(), frame_count, encoded, audio_data.sample_rate, audio_data.channels)) {
		Engine::Helper::ThrowError(isolate, "failed to encode wav output");
		return;
	}

	std::string error;
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
