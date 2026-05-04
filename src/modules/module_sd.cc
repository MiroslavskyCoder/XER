#include "modules/module_builders.h"

#include "content/ai/sd_base/core/sd_base_engine.h"
#include "content/ai/sd_base/core/sd_base_types.h"

#include <algorithm>
#include <cstdint>
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
	}

	bool last_ok() const { return last_ok_; }
	const std::string& last_error() const { return last_error_; }
	const std::map<std::string, std::string>& last_metadata() const { return last_metadata_; }
	const std::vector<uint32_t>& last_image_shape() const { return last_image_shape_; }

private:
	Engine::AI::SDBase::SdBaseEngine engine_;
	v8::Global<v8::Object> packet_;
	bool last_ok_ = false;
	std::string last_error_;
	std::map<std::string, std::string> last_metadata_;
	std::vector<uint32_t> last_image_shape_;
};

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
