#include "modules/module_builders.h"

#include "content/ai/sd_base/core/sd_base_engine.h"
#include "content/ai/sd_base/core/sd_base_types.h"
#include "image/core/image_buffer.h"
#include "image/core/pixel_format.h"
#include "image/io/image_saver.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace modules::detail {
namespace {

class SdSessionStore {
public:
	Engine::AI::SDBase::SdBaseEngine& engine() { return engine_; }

	void SetPacket(v8::Isolate* isolate, v8::Local<v8::Context> context, v8::Local<v8::Object> packet) {
		packet_.Reset(isolate, packet);
	}

	v8::Local<v8::Object> GetPacket(v8::Isolate* isolate) const {
		if (packet_.IsEmpty()) return v8::Object::New(isolate);
		return packet_.Get(isolate);
	}

	void UpdateLastResult(const Engine::AI::SDBase::SdGenerationResult& result) {
		last_ok_ = result.ok;
		last_error_ = result.error;
		last_metadata_ = result.metadata;
		last_image_shape_ = result.image.GetShape();
		last_image_ = result.image;
	}

	bool last_ok() const { return last_ok_; }
	const std::string& last_error() const { return last_error_; }
	const std::map<std::string, std::string>& last_metadata() const { return last_metadata_; }
	const std::vector<uint32_t>& last_image_shape() const { return last_image_shape_; }
	const Engine::MLData::Types::Tensor& last_image() const { return last_image_; }

private:
	Engine::AI::SDBase::SdBaseEngine engine_;
	v8::Global<v8::Object> packet_;
	bool last_ok_ = false;
	std::string last_error_;
	std::map<std::string, std::string> last_metadata_;
	std::vector<uint32_t> last_image_shape_;
	Engine::MLData::Types::Tensor last_image_ = Engine::MLData::Types::Tensor({1, 3, 64, 64});
};

uint8_t ToImageByte(float value) {
	if (value >= 0.0f && value <= 1.0f) {
		return static_cast<uint8_t>(std::round(value * 255.0f));
	}
	return static_cast<uint8_t>(std::clamp<float>(value, 0.0f, 255.0f));
}

bool TensorToRgba(const Engine::MLData::Types::Tensor& tensor,
		  std::vector<uint8_t>* rgba_out,
		  uint32_t* width_out,
		  uint32_t* height_out) {
	using Engine::MLData::Types::DataType;
	if (rgba_out == nullptr || width_out == nullptr || height_out == nullptr) return false;
	if (tensor.GetData() == nullptr) return false;
	const std::vector<uint32_t> shape = tensor.GetShape();

	uint32_t width = 0;
	uint32_t height = 0;
	bool chw = false;
	if (shape.size() == 4 && shape[1] >= 3) {
		height = shape[2];
		width = shape[3];
		chw = true;
	} else if (shape.size() == 3 && shape[0] >= 3) {
		height = shape[1];
		width = shape[2];
		chw = true;
	} else if (shape.size() == 3 && shape[2] >= 3) {
		height = shape[0];
		width = shape[1];
		chw = false;
	} else {
		return false;
	}

	if (width == 0 || height == 0) return false;

	auto read_scalar = [&](size_t index) -> float {
		switch (tensor.GetDataType()) {
			case DataType::FLOAT32:
				return static_cast<const float*>(tensor.GetData())[index];
			case DataType::FLOAT64:
				return static_cast<float>(static_cast<const double*>(tensor.GetData())[index]);
			case DataType::INT32:
				return static_cast<float>(static_cast<const int32_t*>(tensor.GetData())[index]);
			case DataType::INT64:
				return static_cast<float>(static_cast<const int64_t*>(tensor.GetData())[index]);
			case DataType::UINT32:
				return static_cast<float>(static_cast<const uint32_t*>(tensor.GetData())[index]);
			case DataType::UINT8:
				return static_cast<float>(static_cast<const uint8_t*>(tensor.GetData())[index]);
		}
		return 0.0f;
	};

	const size_t plane = static_cast<size_t>(width) * static_cast<size_t>(height);
	rgba_out->assign(plane * 4, 255);
	for (uint32_t y = 0; y < height; ++y) {
		for (uint32_t x = 0; x < width; ++x) {
			const size_t pixel = static_cast<size_t>(y) * width + x;
			float r = 0.0f;
			float g = 0.0f;
			float b = 0.0f;
			if (chw) {
				r = read_scalar(0 * plane + pixel);
				g = read_scalar(1 * plane + pixel);
				b = read_scalar(2 * plane + pixel);
			} else {
				const size_t base = pixel * 3;
				r = read_scalar(base + 0);
				g = read_scalar(base + 1);
				b = read_scalar(base + 2);
			}

			(*rgba_out)[pixel * 4 + 0] = ToImageByte(r);
			(*rgba_out)[pixel * 4 + 1] = ToImageByte(g);
			(*rgba_out)[pixel * 4 + 2] = ToImageByte(b);
			(*rgba_out)[pixel * 4 + 3] = 255;
		}
	}

	*width_out = width;
	*height_out = height;
	return true;
}

v8::Local<v8::Array> MakeByteArray(v8::Isolate* isolate,
				   v8::Local<v8::Context> context,
				   const std::vector<uint8_t>& values) {
	v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(values.size()));
	for (uint32_t i = 0; i < values.size(); ++i) {
		array->Set(context, i, v8::Integer::New(isolate, static_cast<int>(values[i]))).FromMaybe(false);
	}
	return array;
}

v8::Local<v8::Object> MakeRgbaImageObject(v8::Isolate* isolate,
				  v8::Local<v8::Context> context,
				  uint32_t width,
				  uint32_t height,
				  const std::vector<uint8_t>& rgba) {
	v8::Local<v8::Object> image = v8::Object::New(isolate);
	SetProperty(isolate, context, image, "width", v8::Number::New(isolate, static_cast<double>(width)));
	SetProperty(isolate, context, image, "height", v8::Number::New(isolate, static_cast<double>(height)));
	SetProperty(isolate, context, image, "pixelFormat", Engine::Helper::ToV8Str(isolate, "rgba"));
	SetProperty(isolate, context, image, "data", MakeByteArray(isolate, context, rgba));
	return image;
}

SdSessionStore* UnwrapSdSession(const v8::FunctionCallbackInfo<v8::Value>& args) {
	if (!args.This()->IsObject()) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "SDSession receiver is invalid");
		return nullptr;
	}
	SdSessionStore* store = Engine::Helper::UnwrapPointer<SdSessionStore>(args.This().As<v8::Object>());
	if (store == nullptr) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "SDSession is not initialized");
	}
	return store;
}

Engine::AI::SDBase::SdGenerationRequest ParseGenerationRequest(
	v8::Isolate* isolate,
	v8::Local<v8::Context> context,
	v8::Local<v8::Object> options) {
	Engine::AI::SDBase::SdGenerationRequest request;
	request.prompt = GetObjectString(isolate, context, options, "prompt", nullptr, std::string());
	request.negative_prompt = GetObjectString(isolate, context, options, "negativePrompt", "negative_prompt", std::string());

	v8::Local<v8::Value> width;
	if (GetObjectValue(isolate, context, options, "width", nullptr, &width) && width->IsNumber()) {
		request.width = static_cast<uint32_t>(std::max<double>(64.0, width.As<v8::Number>()->Value()));
	}
	v8::Local<v8::Value> height;
	if (GetObjectValue(isolate, context, options, "height", nullptr, &height) && height->IsNumber()) {
		request.height = static_cast<uint32_t>(std::max<double>(64.0, height.As<v8::Number>()->Value()));
	}
	v8::Local<v8::Value> steps;
	if (GetObjectValue(isolate, context, options, "steps", nullptr, &steps) && steps->IsNumber()) {
		request.steps = static_cast<uint32_t>(std::max<double>(1.0, steps.As<v8::Number>()->Value()));
	}
	v8::Local<v8::Value> guidance;
	if (GetObjectValue(isolate, context, options, "guidanceScale", "guidance_scale", &guidance) && guidance->IsNumber()) {
		request.guidance_scale = static_cast<float>(guidance.As<v8::Number>()->Value());
	}
	v8::Local<v8::Value> seed;
	if (GetObjectValue(isolate, context, options, "seed", nullptr, &seed) && seed->IsNumber()) {
		request.seed = static_cast<uint64_t>(std::max<double>(0.0, seed.As<v8::Number>()->Value()));
	}
	const std::string scheduler = GetObjectString(isolate, context, options, "scheduler", nullptr, "euler");
	request.scheduler = (ToLowerCopy(scheduler) == "ddim")
		? Engine::AI::SDBase::SdSchedulerType::DDIM
		: Engine::AI::SDBase::SdSchedulerType::Euler;
	request.enable_sdxl = GetObjectBool(isolate, context, options, "enableSdxl", "enable_sdxl", true);
	request.enable_controlnet =
		GetObjectBool(isolate, context, options, "enableControlNet", "enable_controlnet", true);
	request.enable_vae_decode =
		GetObjectBool(isolate, context, options, "enableVae", "enable_vae", true);
	request.strict_model_loading =
		GetObjectBool(isolate, context, options, "strictModelLoading", "strict_model_loading", false);
	v8::Local<v8::Value> control_strength;
	if (GetObjectValue(isolate, context, options, "controlNetStrength", "controlnet_strength", &control_strength) &&
	    control_strength->IsNumber()) {
		request.controlnet_strength = static_cast<float>(control_strength.As<v8::Number>()->Value());
	}
	v8::Local<v8::Value> depth_strength;
	if (GetObjectValue(isolate, context, options, "depthStrength", "depth_strength", &depth_strength) &&
	    depth_strength->IsNumber()) {
		request.depth_strength = static_cast<float>(depth_strength.As<v8::Number>()->Value());
	}
	request.controlnet_hint =
		GetObjectString(isolate, context, options, "controlHint", "control_hint", std::string());
	request.backend_hint =
		GetObjectString(isolate, context, options, "backend", "backend_hint", std::string());

	v8::Local<v8::Value> model_weights_value;
	if (GetObjectValue(isolate, context, options, "modelWeights", "model_weights", &model_weights_value) &&
	    model_weights_value->IsObject()) {
		v8::Local<v8::Object> mw = model_weights_value.As<v8::Object>();
		request.text_encoder_path =
			GetObjectString(isolate, context, mw, "textEncoder", "text_encoder", std::string());
		request.unet_path =
			GetObjectString(isolate, context, mw, "unet", nullptr, std::string());
		request.vae_decoder_path =
			GetObjectString(isolate, context, mw, "vaeDecoder", "vae_decoder", std::string());
		request.controlnet_path =
			GetObjectString(isolate, context, mw, "controlNet", "controlnet", std::string());
		request.sdxl_text_encoder_2_path =
			GetObjectString(isolate, context, mw, "sdxlTextEncoder2", "sdxl_text_encoder_2", std::string());
		request.sdxl_refiner_unet_path =
			GetObjectString(isolate, context, mw, "sdxlRefinerUnet", "sdxl_refiner_unet", std::string());
	}
	return request;
}

v8::Local<v8::Object> MakeGenerationResultObject(v8::Isolate* isolate,
						  v8::Local<v8::Context> context,
						  const Engine::AI::SDBase::SdGenerationResult& result) {
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "ok", v8::Boolean::New(isolate, result.ok));
	SetProperty(isolate, context, object, "error", Engine::Helper::ToV8Str(isolate, result.error));

	const std::vector<uint32_t> shape = result.image.GetShape();
	v8::Local<v8::Array> shape_array = v8::Array::New(isolate, static_cast<int>(shape.size()));
	for (uint32_t i = 0; i < shape.size(); ++i) {
		shape_array->Set(context, i, v8::Number::New(isolate, static_cast<double>(shape[i]))).FromMaybe(false);
	}
	SetProperty(isolate, context, object, "imageShape", shape_array);

	v8::Local<v8::Object> metadata = v8::Object::New(isolate);
	for (const auto& kv : result.metadata) {
		SetProperty(isolate, context, metadata, kv.first.c_str(), Engine::Helper::ToV8Str(isolate, kv.second));
	}
	SetProperty(isolate, context, object, "metadata", metadata);
	return object;
}

v8::Local<v8::Object> MakeGenerationImageResultObject(v8::Isolate* isolate,
					       v8::Local<v8::Context> context,
					       const Engine::AI::SDBase::SdGenerationResult& result) {
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "generationOk", v8::Boolean::New(isolate, result.ok));
	SetProperty(isolate, context, object, "error", Engine::Helper::ToV8Str(isolate, result.error));

	v8::Local<v8::Object> metadata = v8::Object::New(isolate);
	for (const auto& kv : result.metadata) {
		SetProperty(isolate, context, metadata, kv.first.c_str(), Engine::Helper::ToV8Str(isolate, kv.second));
	}
	SetProperty(isolate, context, object, "metadata", metadata);

	std::vector<uint8_t> rgba;
	uint32_t width = 0;
	uint32_t height = 0;
	const bool converted = result.ok && TensorToRgba(result.image, &rgba, &width, &height);
	SetProperty(isolate, context, object, "ok", v8::Boolean::New(isolate, converted));
	SetProperty(isolate, context, object, "converted", v8::Boolean::New(isolate, converted));
	if (!converted) {
		const std::string error = result.ok ? "failed to convert SD tensor to RGBA image" : result.error;
		SetProperty(isolate, context, object, "error", Engine::Helper::ToV8Str(isolate, error));
		return object;
	}

	SetProperty(isolate, context, object, "width", v8::Number::New(isolate, static_cast<double>(width)));
	SetProperty(isolate, context, object, "height", v8::Number::New(isolate, static_cast<double>(height)));
	SetProperty(isolate, context, object, "pixelFormat", Engine::Helper::ToV8Str(isolate, "rgba"));
	SetProperty(isolate, context, object, "data", MakeByteArray(isolate, context, rgba));
	SetProperty(isolate, context, object, "image", MakeRgbaImageObject(isolate, context, width, height, rgba));
	return object;
}

v8::Local<v8::FunctionTemplate> MakeSdSessionTemplate(v8::Isolate* isolate);

void SdSessionConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	if (!args.IsConstructCall()) {
		v8::Local<v8::Function> ctor = MakeSdSessionTemplate(isolate)->GetFunction(context).ToLocalChecked();
		v8::Local<v8::Object> instance = ctor->NewInstance(context).ToLocalChecked();
		args.GetReturnValue().Set(instance);
		return;
	}

	auto* store = new SdSessionStore();
	Engine::Helper::WrapPointer(args.This(), store);
	Engine::Helper::RegisterWeakCleanup(isolate, args.This(), store);
	args.GetReturnValue().Set(args.This());
}

void SdInfoCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "name", Engine::Helper::ToV8Str(isolate, "SD"));
	SetProperty(isolate, context, object, "version", Engine::Helper::ToV8Str(isolate, "sd_base/1"));
	SetProperty(isolate, context, object, "defaultWidth", v8::Number::New(isolate, 512));
	SetProperty(isolate, context, object, "defaultHeight", v8::Number::New(isolate, 512));
	SetProperty(isolate, context, object, "defaultSteps", v8::Number::New(isolate, 30));
	args.GetReturnValue().Set(object);
}

void SdSessionAttachPacketCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	SdSessionStore* store = UnwrapSdSession(args);
	if (store == nullptr) return;
	if (args.Length() < 1 || !args[0]->IsObject()) {
		Engine::Helper::ThrowTypeError(isolate, "SDSession.attachPacket expects packet object");
		return;
	}
	store->SetPacket(isolate, context, args[0].As<v8::Object>());
	args.GetReturnValue().Set(args.This());
}

void SdSessionGetPacketCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	SdSessionStore* store = UnwrapSdSession(args);
	if (store == nullptr) return;
	args.GetReturnValue().Set(store->GetPacket(args.GetIsolate()));
}

void SdSessionGenerateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	SdSessionStore* store = UnwrapSdSession(args);
	if (store == nullptr) return;

	Engine::AI::SDBase::SdGenerationRequest request;
	if (args.Length() > 0 && args[0]->IsObject()) {
		v8::Local<v8::Object> options = args[0].As<v8::Object>();
		request = ParseGenerationRequest(isolate, context, options);
	}

	// If prompt was omitted, try to pull it from attached AI packet.
	if (request.prompt.empty()) {
		v8::Local<v8::Object> packet = store->GetPacket(isolate);
		v8::Local<v8::Value> maybe_prompt;
		if (GetObjectValue(isolate, context, packet, "prompt", nullptr, &maybe_prompt) && maybe_prompt->IsString()) {
			request.prompt = Engine::Helper::FromV8Str(isolate, maybe_prompt);
		}
	}
	if (request.prompt.empty()) {
		request.prompt = "masterpiece, detailed scene";
	}

	const auto result = store->engine().Generate(request);
	store->UpdateLastResult(result);
	args.GetReturnValue().Set(MakeGenerationResultObject(isolate, context, result));
}

void SdSessionGenerateImageCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	SdSessionStore* store = UnwrapSdSession(args);
	if (store == nullptr) return;

	Engine::AI::SDBase::SdGenerationRequest request;
	if (args.Length() > 0 && args[0]->IsObject()) {
		v8::Local<v8::Object> options = args[0].As<v8::Object>();
		request = ParseGenerationRequest(isolate, context, options);
	}
	if (request.prompt.empty()) {
		v8::Local<v8::Object> packet = store->GetPacket(isolate);
		v8::Local<v8::Value> maybe_prompt;
		if (GetObjectValue(isolate, context, packet, "prompt", nullptr, &maybe_prompt) && maybe_prompt->IsString()) {
			request.prompt = Engine::Helper::FromV8Str(isolate, maybe_prompt);
		}
	}
	if (request.prompt.empty()) {
		request.prompt = "masterpiece, detailed scene";
	}

	const auto result = store->engine().Generate(request);
	store->UpdateLastResult(result);
	args.GetReturnValue().Set(MakeGenerationImageResultObject(isolate, context, result));
}

void SdSessionSaveLastImageCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	SdSessionStore* store = UnwrapSdSession(args);
	if (store == nullptr) return;
	std::string path;
	if (!RequireStringArg(args, 0, "SDSession.saveLastImage expects path", &path)) return;
	if (!store->last_ok()) {
		Engine::Helper::ThrowRangeError(isolate, "no successful generation result in session");
		return;
	}

	std::vector<uint8_t> rgba;
	uint32_t width = 0;
	uint32_t height = 0;
	if (!TensorToRgba(store->last_image(), &rgba, &width, &height)) {
		Engine::Helper::ThrowRangeError(isolate, "failed to convert last SD tensor to image");
		return;
	}

	image::ImageBuffer image_buffer(static_cast<int>(width), static_cast<int>(height), image::PixelFormat::RGBA8, image::ColorSpace::sRGB);
	std::copy(rgba.begin(), rgba.end(), image_buffer.Data());
	image::ImageSaver saver;
	args.GetReturnValue().Set(v8::Boolean::New(isolate, saver.Save(image_buffer, path, 90)));
}

void SdSessionLastResultCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	SdSessionStore* store = UnwrapSdSession(args);
	if (store == nullptr) return;

	v8::Local<v8::Object> obj = v8::Object::New(isolate);
	SetProperty(isolate, context, obj, "ok", v8::Boolean::New(isolate, store->last_ok()));
	SetProperty(isolate, context, obj, "error", Engine::Helper::ToV8Str(isolate, store->last_error()));

	v8::Local<v8::Array> shape = v8::Array::New(isolate, static_cast<int>(store->last_image_shape().size()));
	for (uint32_t i = 0; i < store->last_image_shape().size(); ++i) {
		shape->Set(context, i, v8::Number::New(isolate, static_cast<double>(store->last_image_shape()[i]))).FromMaybe(false);
	}
	SetProperty(isolate, context, obj, "imageShape", shape);

	v8::Local<v8::Object> metadata = v8::Object::New(isolate);
	for (const auto& kv : store->last_metadata()) {
		SetProperty(isolate, context, metadata, kv.first.c_str(), Engine::Helper::ToV8Str(isolate, kv.second));
	}
	SetProperty(isolate, context, obj, "metadata", metadata);
	args.GetReturnValue().Set(obj);
}

v8::Local<v8::FunctionTemplate> MakeSdSessionTemplate(v8::Isolate* isolate) {
	return Engine::Helper::MakeClass(
		isolate,
		"SDSession",
		&SdSessionConstructor,
		{{"attachPacket", &SdSessionAttachPacketCallback},
		 {"getPacket", &SdSessionGetPacketCallback},
		 {"generate", &SdSessionGenerateCallback},
		 {"generateImage", &SdSessionGenerateImageCallback},
		 {"saveLastImage", &SdSessionSaveLastImageCallback},
		 {"lastResult", &SdSessionLastResultCallback}});
}

void SdCreateSessionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Function> ctor = MakeSdSessionTemplate(isolate)->GetFunction(context).ToLocalChecked();
	v8::Local<v8::Object> instance = ctor->NewInstance(context).ToLocalChecked();
	args.GetReturnValue().Set(instance);
}

void SdGenerateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	Engine::AI::SDBase::SdBaseEngine engine;

	Engine::AI::SDBase::SdGenerationRequest request;
	if (args.Length() > 0 && args[0]->IsObject()) {
		request = ParseGenerationRequest(isolate, context, args[0].As<v8::Object>());
	}
	if (request.prompt.empty()) request.prompt = "masterpiece, detailed scene";

	const auto result = engine.Generate(request);
	args.GetReturnValue().Set(MakeGenerationResultObject(isolate, context, result));
}

void SdGenerateImageCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	Engine::AI::SDBase::SdBaseEngine engine;

	Engine::AI::SDBase::SdGenerationRequest request;
	if (args.Length() > 0 && args[0]->IsObject()) {
		request = ParseGenerationRequest(isolate, context, args[0].As<v8::Object>());
	}
	if (request.prompt.empty()) request.prompt = "masterpiece, detailed scene";

	const auto result = engine.Generate(request);
	args.GetReturnValue().Set(MakeGenerationImageResultObject(isolate, context, result));
}

}  // namespace

bool BuildSdModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	v8::Local<v8::Function> session_ctor = MakeSdSessionTemplate(isolate)->GetFunction(context).ToLocalChecked();
	bool ok = SetProperty(isolate, context, module, "name", Engine::Helper::ToV8Str(isolate, "SD"));
	ok = ok && SetProperty(isolate, context, module, "available", v8::Boolean::New(isolate, true));
	ok = ok && SetProperty(isolate, context, module, "summary",
			       Engine::Helper::ToV8Str(isolate, "Stable Diffusion base pipeline"));
	ok = ok && SetProperty(isolate, context, module, "SDSession", session_ctor);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "info", &SdInfoCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "createSession", &SdCreateSessionCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "generate", &SdGenerateCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "generateImage", &SdGenerateImageCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build SD module";
		}
		return false;
	}

	*module_out = module;
	return true;
}

}  // namespace modules::detail
