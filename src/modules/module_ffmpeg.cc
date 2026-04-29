#include "modules/module_bridge_common.h"
#include "modules/module_builders.h"

#include "wrapper/ffmpeg/ffmpeg_engine_bridge.h"

namespace modules::detail {
namespace {

v8::Local<v8::Object> MakeFFmpegMediaInfoObject(v8::Isolate* isolate,
						v8::Local<v8::Context> context,
						const engine::bridge::ffmpeg::MediaInfo& info) {
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	v8::Local<v8::Array> streams = v8::Array::New(isolate, static_cast<int>(info.streams.size()));
	SetProperty(isolate, context, object, "path", Engine::Helper::ToV8Str(isolate, info.path));
	SetProperty(isolate, context, object, "formatName", Engine::Helper::ToV8Str(isolate, info.format_name));
	SetProperty(isolate, context, object, "formatLongName", Engine::Helper::ToV8Str(isolate, info.format_long_name));
	SetProperty(isolate, context, object, "duration", v8::Number::New(isolate, static_cast<double>(info.duration)));
	SetProperty(isolate, context, object, "size", v8::Number::New(isolate, static_cast<double>(info.size)));
	SetProperty(isolate, context, object, "bitRate", v8::Number::New(isolate, static_cast<double>(info.bit_rate)));
	for (size_t index = 0; index < info.streams.size(); ++index) {
		const auto& stream = info.streams[index];
		v8::Local<v8::Object> stream_object = v8::Object::New(isolate);
		SetProperty(isolate, context, stream_object, "index", v8::Integer::New(isolate, stream.index));
		SetProperty(isolate, context, stream_object, "mediaType", Engine::Helper::ToV8Str(isolate, stream.media_type));
		SetProperty(isolate, context, stream_object, "codecName", Engine::Helper::ToV8Str(isolate, stream.codec_name));
		SetProperty(isolate, context, stream_object, "codecLongName", Engine::Helper::ToV8Str(isolate, stream.codec_long_name));
		SetProperty(isolate, context, stream_object, "width", v8::Integer::New(isolate, stream.width));
		SetProperty(isolate, context, stream_object, "height", v8::Integer::New(isolate, stream.height));
		SetProperty(isolate, context, stream_object, "sampleRate", v8::Integer::New(isolate, stream.sample_rate));
		SetProperty(isolate, context, stream_object, "channels", v8::Integer::New(isolate, stream.channels));
		SetProperty(isolate, context, stream_object, "frameCount", v8::Number::New(isolate, static_cast<double>(stream.frame_count)));
		SetProperty(isolate, context, stream_object, "language", Engine::Helper::ToV8Str(isolate, stream.language));
		streams->Set(context, static_cast<uint32_t>(index), stream_object).FromMaybe(false);
	}
	SetProperty(isolate, context, object, "streams", streams);
	return object;
}

void FFmpegProbeMediaCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string path;
	if (!RequireStringArg(args, 0, "probeMedia expects input path string", &path)) {
		return;
	}
	engine::bridge::ffmpeg::MediaInfo info;
	std::string error;
	if (!engine::bridge::ffmpeg::ProbeMedia(path, &info, &error)) {
		Engine::Helper::ThrowError(isolate, error.empty() ? "ProbeMedia failed" : error);
		return;
	}
	args.GetReturnValue().Set(MakeFFmpegMediaInfoObject(isolate, context, info));
}

}  // namespace

bool BuildFFmpegModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = SetBridgeStatusProperties(
		isolate,
		context,
		module,
		"FFmpeg",
		engine::bridge::ffmpeg::IsAvailable(),
		engine::bridge::ffmpeg::Summary(),
		error_out);
	ok = ok && SetProperty(isolate, context, module, "configuration", Engine::Helper::ToV8Str(isolate, engine::bridge::ffmpeg::Configuration()));
	ok = ok && SetProperty(isolate, context, module, "license", Engine::Helper::ToV8Str(isolate, engine::bridge::ffmpeg::License()));
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "probeMedia", &FFmpegProbeMediaCallback);
	if (!ok) {
		if (error_out != nullptr && error_out->empty()) {
			*error_out = "failed to build FFmpeg module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail