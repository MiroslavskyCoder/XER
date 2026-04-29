#include "modules/module_builders.h"

#include "wrapper/angle/angle_engine_bridge.h"
#include "wrapper/cuda/cuda_engine_bridge.h"
#include "wrapper/cudnn/cudnn_engine_bridge.h"
#include "wrapper/ffmpeg/ffmpeg_engine_bridge.h"
#include "wrapper/opencv/opencv_engine_bridge.h"
#include "wrapper/skia/skia_engine_bridge.h"

#include <string>
#include <vector>

namespace modules::detail {
namespace {

bool BuildStatusModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  const std::string& name,
			  bool available,
			  const std::string& summary,
			  v8::Local<v8::Object>* module_out,
			  std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	const bool ok = SetProperty(isolate, context, module, "name", Engine::Helper::ToV8Str(isolate, name))
		&& SetProperty(isolate, context, module, "available", v8::Boolean::New(isolate, available))
		&& SetProperty(isolate, context, module, "summary", Engine::Helper::ToV8Str(isolate, summary));
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build module: " + name;
		}
		return false;
	}
	*module_out = module;
	return true;
}

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

bool BuildOpenCvModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out) {
	return BuildStatusModule(isolate, context, "OpenCV", engine::bridge::opencv::IsAvailable(), engine::bridge::opencv::Summary(), module_out, error_out);
}

bool BuildCudaModule(v8::Isolate* isolate,
			 v8::Local<v8::Context> context,
			 v8::Local<v8::Object>* module_out,
			 std::string* error_out) {
	return BuildStatusModule(isolate, context, "CUDA", engine::bridge::cuda::IsAvailable(), engine::bridge::cuda::Summary(), module_out, error_out);
}

bool BuildCudnnModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  v8::Local<v8::Object>* module_out,
			  std::string* error_out) {
	return BuildStatusModule(isolate, context, "CUDNN", engine::bridge::cudnn::IsAvailable(), engine::bridge::cudnn::Summary(), module_out, error_out);
}

bool BuildAngleModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  v8::Local<v8::Object>* module_out,
			  std::string* error_out) {
	return BuildStatusModule(isolate, context, "ANGLE", engine::bridge::angle::IsAvailable(), engine::bridge::angle::Summary(), module_out, error_out);
}

bool BuildVtkModule(v8::Isolate* isolate,
			v8::Local<v8::Context> context,
			v8::Local<v8::Object>* module_out,
			std::string* error_out) {
	return BuildStatusModule(isolate, context, "VTK", false, "VTK bridge unavailable in this build", module_out, error_out);
}

bool BuildSkiaModule(v8::Isolate* isolate,
			 v8::Local<v8::Context> context,
			 v8::Local<v8::Object>* module_out,
			 std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && SetProperty(isolate, context, module, "name", Engine::Helper::ToV8Str(isolate, "Skia"));
	ok = ok && SetProperty(isolate, context, module, "available", v8::Boolean::New(isolate, engine::bridge::skia::IsAvailable()));
	ok = ok && SetProperty(isolate, context, module, "summary", Engine::Helper::ToV8Str(isolate, engine::bridge::skia::Summary()));
	ok = ok && SetProperty(isolate, context, module, "version", Engine::Helper::ToV8Str(isolate, engine::bridge::skia::Version()));
	ok = ok && SetProperty(isolate, context, module, "exportedFunctions", MakeStringArray(isolate, context, engine::bridge::skia::ExportedFunctionNames()));
	ok = ok && SetProperty(isolate, context, module, "blendModes", MakeStringArray(isolate, context, engine::bridge::skia::BlendModeNames()));
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Skia module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

bool BuildFFmpegModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && SetProperty(isolate, context, module, "name", Engine::Helper::ToV8Str(isolate, "FFmpeg"));
	ok = ok && SetProperty(isolate, context, module, "available", v8::Boolean::New(isolate, engine::bridge::ffmpeg::IsAvailable()));
	ok = ok && SetProperty(isolate, context, module, "summary", Engine::Helper::ToV8Str(isolate, engine::bridge::ffmpeg::Summary()));
	ok = ok && SetProperty(isolate, context, module, "configuration", Engine::Helper::ToV8Str(isolate, engine::bridge::ffmpeg::Configuration()));
	ok = ok && SetProperty(isolate, context, module, "license", Engine::Helper::ToV8Str(isolate, engine::bridge::ffmpeg::License()));
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "probeMedia", &FFmpegProbeMediaCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build FFmpeg module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail