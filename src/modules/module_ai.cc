#include "modules/module_builders.h"

#include "content/ai/models_builder/utility/ai_runtime_features.h"
#include "content/ai/models_builder/utility/mb_logger.h"
#include "wrapper/cuda/cuda_engine_bridge.h"
#include "wrapper/ffmpeg/ffmpeg_engine_bridge.h"
#include "wrapper/opencv/opencv_engine_bridge.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace modules::detail {
namespace {

struct AiTensorData {
	std::vector<uint32_t> shape;
	std::vector<float> values;
};

class AiContextStore {
public:
	void SetMeta(std::string key, std::string value) { metadata_[std::move(key)] = std::move(value); }

	std::string GetMeta(const std::string& key) const {
		auto it = metadata_.find(key);
		return it == metadata_.end() ? std::string() : it->second;
	}

	std::vector<std::string> MetaKeys() const {
		std::vector<std::string> keys;
		keys.reserve(metadata_.size());
		for (const auto& kv : metadata_) keys.push_back(kv.first);
		return keys;
	}

	void PutTensor(std::string name, AiTensorData tensor) { tensors_[std::move(name)] = std::move(tensor); }

	const AiTensorData* GetTensor(const std::string& name) const {
		auto it = tensors_.find(name);
		return it == tensors_.end() ? nullptr : &it->second;
	}

	std::vector<std::string> TensorNames() const {
		std::vector<std::string> names;
		names.reserve(tensors_.size());
		for (const auto& kv : tensors_) names.push_back(kv.first);
		return names;
	}

	void Clear() {
		tensors_.clear();
		metadata_.clear();
	}

	const std::unordered_map<std::string, AiTensorData>& tensors() const { return tensors_; }

private:
	std::unordered_map<std::string, AiTensorData> tensors_;
	std::unordered_map<std::string, std::string> metadata_;
};

AiContextStore* UnwrapAiContext(const v8::FunctionCallbackInfo<v8::Value>& args) {
	if (!args.This()->IsObject()) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AIContext receiver is invalid");
		return nullptr;
	}
	AiContextStore* store = Engine::Helper::UnwrapPointer<AiContextStore>(args.This().As<v8::Object>());
	if (store == nullptr) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AIContext is not initialized");
	}
	return store;
}

bool ReadUIntArray(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Value> value,
			   std::vector<uint32_t>* out) {
	if (out == nullptr || !value->IsArray()) return false;
	v8::Local<v8::Array> arr = value.As<v8::Array>();
	out->clear();
	out->reserve(arr->Length());
	for (uint32_t i = 0; i < arr->Length(); ++i) {
		v8::Local<v8::Value> v;
		if (!arr->Get(context, i).ToLocal(&v) || !v->IsNumber()) return false;
		const double n = v.As<v8::Number>()->Value();
		out->push_back(static_cast<uint32_t>(std::max<double>(0.0, n)));
	}
	return true;
}

bool ReadFloatArray(v8::Isolate* isolate,
			v8::Local<v8::Context> context,
			v8::Local<v8::Value> value,
			std::vector<float>* out) {
	if (out == nullptr || !value->IsArray()) return false;
	v8::Local<v8::Array> arr = value.As<v8::Array>();
	out->clear();
	out->reserve(arr->Length());
	for (uint32_t i = 0; i < arr->Length(); ++i) {
		v8::Local<v8::Value> v;
		if (!arr->Get(context, i).ToLocal(&v) || !v->IsNumber()) return false;
		out->push_back(static_cast<float>(v.As<v8::Number>()->Value()));
	}
	return true;
}

v8::Local<v8::Object> MakeTensorObject(v8::Isolate* isolate,
				       v8::Local<v8::Context> context,
				       const std::string& name,
				       const AiTensorData& tensor,
				       bool include_data) {
	v8::Local<v8::Object> obj = v8::Object::New(isolate);
	SetProperty(isolate, context, obj, "name", Engine::Helper::ToV8Str(isolate, name));

	v8::Local<v8::Array> shape = v8::Array::New(isolate, static_cast<int>(tensor.shape.size()));
	for (uint32_t i = 0; i < tensor.shape.size(); ++i) {
		shape->Set(context, i, v8::Number::New(isolate, static_cast<double>(tensor.shape[i]))).FromMaybe(false);
	}
	SetProperty(isolate, context, obj, "shape", shape);

	SetProperty(isolate, context, obj, "elementCount",
			v8::Number::New(isolate, static_cast<double>(tensor.values.size())));

	if (include_data) {
		v8::Local<v8::Array> data = v8::Array::New(isolate, static_cast<int>(tensor.values.size()));
		for (uint32_t i = 0; i < tensor.values.size(); ++i) {
			data->Set(context, i, v8::Number::New(isolate, static_cast<double>(tensor.values[i]))).FromMaybe(false);
		}
		SetProperty(isolate, context, obj, "data", data);
	}

	return obj;
}

v8::Local<v8::Object> MakeRuntimeFeaturesObject(
	v8::Isolate* isolate,
	v8::Local<v8::Context> context,
	const Engine::ModelsBuilder::Utility::ExternalLibraryAvailability& libs) {
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "rangeV3", v8::Boolean::New(isolate, libs.has_range_v3));
	SetProperty(isolate, context, object, "absl", v8::Boolean::New(isolate, libs.has_absl));
	SetProperty(isolate, context, object, "zlib", v8::Boolean::New(isolate, libs.has_zlib));
	SetProperty(isolate, context, object, "icu", v8::Boolean::New(isolate, libs.has_icu));
	SetProperty(isolate, context, object, "libuv", v8::Boolean::New(isolate, libs.has_libuv));
	SetProperty(isolate, context, object, "cuda", v8::Boolean::New(isolate, libs.has_cuda));
	SetProperty(isolate, context, object, "cudnn", v8::Boolean::New(isolate, libs.has_cudnn));
	SetProperty(isolate, context, object, "cutlass", v8::Boolean::New(isolate, libs.has_cutlass));
	SetProperty(isolate, context, object, "eigen", v8::Boolean::New(isolate, libs.has_eigen));
	SetProperty(isolate, context, object, "opencv", v8::Boolean::New(isolate, libs.has_opencv));
	SetProperty(isolate, context, object, "xnnpack", v8::Boolean::New(isolate, libs.has_xnnpack));
	SetProperty(isolate, context, object, "flatbuffers", v8::Boolean::New(isolate, libs.has_flatbuffers));
	SetProperty(isolate, context, object, "openvino", v8::Boolean::New(isolate, libs.has_openvino));
	SetProperty(isolate, context, object, "onnx", v8::Boolean::New(isolate, libs.has_onnx));
	SetProperty(isolate, context, object, "tensorflow", v8::Boolean::New(isolate, libs.has_tensorflow));
	SetProperty(isolate, context, object, "pthreadpool", v8::Boolean::New(isolate, libs.has_pthreadpool));
	SetProperty(isolate, context, object, "fp16", v8::Boolean::New(isolate, libs.has_fp16));
	return object;
}

v8::Local<v8::FunctionTemplate> MakeAiContextTemplate(v8::Isolate* isolate);

void AiContextConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	if (!args.IsConstructCall()) {
		v8::Local<v8::Function> ctor = MakeAiContextTemplate(isolate)->GetFunction(context).ToLocalChecked();
		v8::Local<v8::Object> instance = ctor->NewInstance(context).ToLocalChecked();
		args.GetReturnValue().Set(instance);
		return;
	}

	auto* store = new AiContextStore();
	Engine::Helper::WrapPointer(args.This(), store);
	Engine::Helper::RegisterWeakCleanup(isolate, args.This(), store);
	args.GetReturnValue().Set(args.This());
}

void AiContextSetMetaCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	AiContextStore* store = UnwrapAiContext(args);
	if (store == nullptr) return;

	std::string key;
	std::string value;
	if (!RequireStringArg(args, 0, "AIContext.setMeta expects key", &key) ||
	    !RequireStringArg(args, 1, "AIContext.setMeta expects value", &value)) {
		return;
	}
	store->SetMeta(std::move(key), std::move(value));
	args.GetReturnValue().Set(args.This());
}

void AiContextGetMetaCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	AiContextStore* store = UnwrapAiContext(args);
	if (store == nullptr) return;

	std::string key;
	if (!RequireStringArg(args, 0, "AIContext.getMeta expects key", &key)) return;
	const std::string value = store->GetMeta(key);
	if (!value.empty()) {
		args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), value));
		return;
	}
	if (args.Length() > 1) {
		args.GetReturnValue().Set(args[1]);
		return;
	}
	args.GetReturnValue().Set(v8::Undefined(args.GetIsolate()));
}

void AiContextMetaKeysCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	AiContextStore* store = UnwrapAiContext(args);
	if (store == nullptr) return;
	args.GetReturnValue().Set(MakeStringArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), store->MetaKeys()));
}

void AiContextPutTensorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	AiContextStore* store = UnwrapAiContext(args);
	if (store == nullptr) return;

	std::string name;
	if (!RequireStringArg(args, 0, "AIContext.putTensor expects name", &name)) return;
	if (args.Length() < 3) {
		Engine::Helper::ThrowTypeError(isolate, "AIContext.putTensor expects shape and data arrays");
		return;
	}

	AiTensorData tensor;
	if (!ReadUIntArray(isolate, context, args[1], &tensor.shape) ||
	    !ReadFloatArray(isolate, context, args[2], &tensor.values)) {
		Engine::Helper::ThrowTypeError(isolate, "invalid shape/data arrays");
		return;
	}

	uint64_t expected = 1;
	for (uint32_t dim : tensor.shape) expected *= static_cast<uint64_t>(std::max<uint32_t>(1, dim));
	if (expected != tensor.values.size()) {
		Engine::Helper::ThrowRangeError(isolate, "tensor data length does not match shape product");
		return;
	}

	store->PutTensor(std::move(name), std::move(tensor));
	args.GetReturnValue().Set(args.This());
}

void AiContextCreateEmbeddingCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	AiContextStore* store = UnwrapAiContext(args);
	if (store == nullptr) return;

	std::string prompt;
	if (!RequireStringArg(args, 0, "AIContext.createEmbedding expects prompt", &prompt)) return;
	const int dim = (args.Length() > 1 && args[1]->IsNumber())
		? static_cast<int>(std::max<double>(1.0, args[1].As<v8::Number>()->Value()))
		: 768;

	AiTensorData tensor;
	tensor.shape = {1u, static_cast<uint32_t>(dim)};
	tensor.values.resize(static_cast<size_t>(dim), 0.0f);
	const uint64_t seed = std::hash<std::string>{}(prompt);
	for (int i = 0; i < dim; ++i) {
		const uint64_t x = seed ^ (static_cast<uint64_t>(i) * 0x9e3779b97f4a7c15ULL);
		tensor.values[static_cast<size_t>(i)] = static_cast<float>((x & 0xFFFFULL) / 32768.0 - 1.0);
	}

	const std::string name = "embedding:" + std::to_string(seed);
	store->PutTensor(name, tensor);
	args.GetReturnValue().Set(MakeTensorObject(isolate, context, name, tensor, true));
}

void AiContextGetTensorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	AiContextStore* store = UnwrapAiContext(args);
	if (store == nullptr) return;

	std::string name;
	if (!RequireStringArg(args, 0, "AIContext.getTensor expects name", &name)) return;
	const bool include_data = (args.Length() > 1) ? args[1]->BooleanValue(isolate) : true;
	const AiTensorData* tensor = store->GetTensor(name);
	if (tensor == nullptr) {
		args.GetReturnValue().Set(v8::Null(isolate));
		return;
	}
	args.GetReturnValue().Set(MakeTensorObject(isolate, context, name, *tensor, include_data));
}

void AiContextListTensorsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	AiContextStore* store = UnwrapAiContext(args);
	if (store == nullptr) return;
	args.GetReturnValue().Set(MakeStringArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), store->TensorNames()));
}

void AiContextExportPacketCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	AiContextStore* store = UnwrapAiContext(args);
	if (store == nullptr) return;

	v8::Local<v8::Object> packet = v8::Object::New(isolate);
	v8::Local<v8::Object> tensors = v8::Object::New(isolate);

	for (const auto& kv : store->tensors()) {
		SetProperty(isolate, context, tensors, kv.first.c_str(), MakeTensorObject(isolate, context, kv.first, kv.second, true));
	}

	SetProperty(isolate, context, packet, "type", Engine::Helper::ToV8Str(isolate, "ai.packet.v1"));
	SetProperty(isolate, context, packet, "tensors", tensors);
	SetProperty(isolate, context, packet, "metaKeys", MakeStringArray(isolate, context, store->MetaKeys()));
	args.GetReturnValue().Set(packet);
}

void AiContextImportPacketCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	AiContextStore* store = UnwrapAiContext(args);
	if (store == nullptr) return;
	if (args.Length() < 1 || !args[0]->IsObject()) {
		Engine::Helper::ThrowTypeError(isolate, "AIContext.importPacket expects object");
		return;
	}

	v8::Local<v8::Object> packet = args[0].As<v8::Object>();
	v8::Local<v8::Value> tensors_value;
	if (!GetObjectValue(isolate, context, packet, "tensors", nullptr, &tensors_value) || !tensors_value->IsObject()) {
		Engine::Helper::ThrowTypeError(isolate, "packet.tensors must be object");
		return;
	}

	v8::Local<v8::Object> tensors = tensors_value.As<v8::Object>();
	v8::Local<v8::Array> keys;
	if (!tensors->GetOwnPropertyNames(context).ToLocal(&keys)) {
		Engine::Helper::ThrowError(isolate, "failed to iterate packet tensors");
		return;
	}

	for (uint32_t i = 0; i < keys->Length(); ++i) {
		v8::Local<v8::Value> key;
		v8::Local<v8::Value> val;
		if (!keys->Get(context, i).ToLocal(&key) || !tensors->Get(context, key).ToLocal(&val) || !val->IsObject()) {
			continue;
		}
		const std::string name = Engine::Helper::FromV8Str(isolate, key);
		v8::Local<v8::Object> tensor_obj = val.As<v8::Object>();

		v8::Local<v8::Value> shape_v;
		v8::Local<v8::Value> data_v;
		if (!GetObjectValue(isolate, context, tensor_obj, "shape", nullptr, &shape_v) ||
		    !GetObjectValue(isolate, context, tensor_obj, "data", nullptr, &data_v)) {
			continue;
		}

		AiTensorData tensor;
		if (!ReadUIntArray(isolate, context, shape_v, &tensor.shape) ||
		    !ReadFloatArray(isolate, context, data_v, &tensor.values)) {
			continue;
		}
		store->PutTensor(name, std::move(tensor));
	}

	args.GetReturnValue().Set(args.This());
}

void AiContextClearCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	AiContextStore* store = UnwrapAiContext(args);
	if (store == nullptr) return;
	store->Clear();
	args.GetReturnValue().Set(args.This());
}

v8::Local<v8::FunctionTemplate> MakeAiContextTemplate(v8::Isolate* isolate) {
	return Engine::Helper::MakeClass(
		isolate,
		"AIContext",
		&AiContextConstructor,
		{{"setMeta", &AiContextSetMetaCallback},
		 {"getMeta", &AiContextGetMetaCallback},
		 {"metaKeys", &AiContextMetaKeysCallback},
		 {"putTensor", &AiContextPutTensorCallback},
		 {"createEmbedding", &AiContextCreateEmbeddingCallback},
		 {"getTensor", &AiContextGetTensorCallback},
		 {"listTensors", &AiContextListTensorsCallback},
		 {"exportPacket", &AiContextExportPacketCallback},
		 {"importPacket", &AiContextImportPacketCallback},
		 {"clear", &AiContextClearCallback}});
}

void AiInfoCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();

	const auto libs = Engine::ModelsBuilder::Utility::DetectExternalLibraries();
	v8::Local<v8::Object> info = v8::Object::New(isolate);
	SetProperty(isolate, context, info, "name", Engine::Helper::ToV8Str(isolate, "AI"));
	SetProperty(isolate, context, info, "runtimeBanner",
			Engine::Helper::ToV8Str(isolate, Engine::ModelsBuilder::Utility::BuildRuntimeBanner()));
	SetProperty(isolate, context, info, "suggestedThreads",
			v8::Number::New(isolate, static_cast<double>(Engine::ModelsBuilder::Utility::SuggestedInferenceThreadCount())));
	SetProperty(isolate, context, info, "bridges", MakeRuntimeFeaturesObject(isolate, context, libs));
	args.GetReturnValue().Set(info);
}

void AiRuntimeFeaturesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	const auto libs = Engine::ModelsBuilder::Utility::DetectExternalLibraries();
	args.GetReturnValue().Set(MakeRuntimeFeaturesObject(isolate, context, libs));
}

void AiCreateContextCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Function> ctor = MakeAiContextTemplate(isolate)->GetFunction(context).ToLocalChecked();
	v8::Local<v8::Object> instance = ctor->NewInstance(context).ToLocalChecked();
	args.GetReturnValue().Set(instance);
}

}  // namespace

bool BuildAiModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	v8::Local<v8::Function> ai_context_ctor = MakeAiContextTemplate(isolate)->GetFunction(context).ToLocalChecked();
	const bool available = engine::bridge::cuda::IsAvailable() ||
			       engine::bridge::opencv::IsAvailable() ||
			       engine::bridge::ffmpeg::IsAvailable();

	bool ok = SetProperty(isolate, context, module, "name", Engine::Helper::ToV8Str(isolate, "AI"));
	ok = ok && SetProperty(isolate, context, module, "available", v8::Boolean::New(isolate, available));
	ok = ok && SetProperty(isolate, context, module, "summary",
			       Engine::Helper::ToV8Str(isolate, Engine::ModelsBuilder::Utility::BuildRuntimeBanner()));
	ok = ok && SetProperty(isolate, context, module, "AIContext", ai_context_ctor);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "info", &AiInfoCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "runtimeFeatures", &AiRuntimeFeaturesCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "createContext", &AiCreateContextCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build AI module";
		}
		return false;
	}

	Engine::ModelsBuilder::Utility::ModelBuilderLogger::GetInstance().Info("JS module AI initialized");
	*module_out = module;
	return true;
}

}  // namespace modules::detail
