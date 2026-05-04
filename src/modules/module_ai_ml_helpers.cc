#include "modules/module_ai_ml_helpers.h"

#include "content/ai/ml/data_augmentation/augmentor.h"
#include "content/ai/ml/data_caching/cache.h"
#include "content/ai/ml/data_loading/csv_loader.h"
#include "content/ai/ml/data_loading/data_loader.h"
#include "content/ai/ml/data_loading/image_loader.h"
#include "content/ai/ml/data_preprocessing/normalizer.h"
#include "content/ai/ml/data_types/dataset.h"
#include "content/ai/ml/data_types/tensor.h"
#include "content/ai/ml/data_validation/validator.h"
#include "content/ai/ml/features/feat_math_ops.h"
#include "content/ai/ml/utils/ml_config_parser.h"
#include "content/ai/ml/utils/ml_logger.h"
#include "content/ai/ml/utils/ml_math_utils.h"
#include "content/ai/ml/utils/ml_profiler.h"
#include "content/ai/ml/utils/ml_random_generator.h"
#include "content/ai/ml/utils/ml_type_converter.h"
#include "content/ai/ml/utils/ml_version_manager.h"
#include "modules/module_common.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace modules::detail {
namespace {

using Engine::ML::DataAugmentation::AugmentationPipeline;
using Engine::ML::DataAugmentation::ColorJitterAugmentor;
using Engine::ML::DataAugmentation::CropAugmentor;
using Engine::ML::DataAugmentation::FlipAugmentor;
using Engine::ML::DataAugmentation::NoiseAugmentor;
using Engine::ML::DataAugmentation::RotationAugmentor;
using Engine::ML::Features::FeatMathOps;
using Engine::ML::Utils::MlConfigParser;
using Engine::ML::Utils::MlLogEntry;
using Engine::ML::Utils::MlLogLevel;
using Engine::ML::Utils::MlLogger;
using Engine::ML::Utils::MlMathUtils;
using Engine::ML::Utils::MlProfiler;
using Engine::ML::Utils::MlRandomGenerator;
using Engine::ML::Utils::MlTypeConverter;
using Engine::ML::Utils::MlVersionManager;
using Engine::MLData::Caching::DataCache;
using Engine::MLData::Loading::CSVLoader;
using Engine::MLData::Loading::DataLoader;
using Engine::MLData::Loading::ImageLoader;
using Engine::MLData::Preprocessing::Normalizer;
using Engine::MLData::Types::DataType;
using Engine::MLData::Types::Dataset;
using Engine::MLData::Types::Tensor;
using Engine::MLData::Validation::DataValidator;
using Engine::MLData::Validation::ValidationResult;

struct AiTensorData {
	std::vector<uint32_t> shape;
	std::vector<float> values;
};

MlProfiler& GlobalAiProfiler() {
	static MlProfiler profiler;
	return profiler;
}

MlRandomGenerator& GlobalRandom() {
	static MlRandomGenerator generator;
	return generator;
}

std::unordered_map<std::string, std::shared_ptr<Dataset>>& LoadedDatasets() {
	static std::unordered_map<std::string, std::shared_ptr<Dataset>> datasets;
	return datasets;
}

uint64_t& LoadedDatasetCounter() {
	static uint64_t counter = 0;
	return counter;
}

std::string NextDatasetHandle() {
	return "dataset:" + std::to_string(++LoadedDatasetCounter());
}

const char* ToMlLogLevelName(MlLogLevel level) {
	switch (level) {
		case MlLogLevel::Debug: return "debug";
		case MlLogLevel::Info: return "info";
		case MlLogLevel::Warning: return "warning";
		case MlLogLevel::Error: return "error";
	}
	return "info";
}

MlLogLevel ParseMlLogLevel(const std::string& text) {
	const std::string lowered = ToLowerCopy(text);
	if (lowered == "debug") return MlLogLevel::Debug;
	if (lowered == "warning" || lowered == "warn") return MlLogLevel::Warning;
	if (lowered == "error") return MlLogLevel::Error;
	return MlLogLevel::Info;
}

const char* ToTensorDataTypeName(DataType dtype) {
	switch (dtype) {
		case DataType::FLOAT32: return "float32";
		case DataType::FLOAT64: return "float64";
		case DataType::INT32: return "int32";
		case DataType::INT64: return "int64";
		case DataType::UINT32: return "uint32";
		case DataType::UINT8: return "uint8";
	}
	return "float32";
}

DataType ParseTensorDataType(const std::string& text) {
	const std::string lowered = ToLowerCopy(text);
	if (lowered == "float64" || lowered == "double") return DataType::FLOAT64;
	if (lowered == "int32") return DataType::INT32;
	if (lowered == "int64") return DataType::INT64;
	if (lowered == "uint32") return DataType::UINT32;
	if (lowered == "uint8" || lowered == "byte") return DataType::UINT8;
	return DataType::FLOAT32;
}

bool ReadUIntArray(v8::Local<v8::Context> context,
			   v8::Local<v8::Value> value,
			   std::vector<uint32_t>* out) {
	if (out == nullptr || !value->IsArray()) return false;
	v8::Local<v8::Array> arr = value.As<v8::Array>();
	out->clear();
	out->reserve(arr->Length());
	for (uint32_t i = 0; i < arr->Length(); ++i) {
		v8::Local<v8::Value> v;
		if (!arr->Get(context, i).ToLocal(&v) || !v->IsNumber()) return false;
		out->push_back(static_cast<uint32_t>(std::max<double>(0.0, v.As<v8::Number>()->Value())));
	}
	return true;
}

bool ReadFloatArray(v8::Local<v8::Context> context,
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

bool ReadIntArray(v8::Local<v8::Context> context,
			  v8::Local<v8::Value> value,
			  std::vector<int32_t>* out) {
	if (out == nullptr || !value->IsArray()) return false;
	v8::Local<v8::Array> arr = value.As<v8::Array>();
	out->clear();
	out->reserve(arr->Length());
	for (uint32_t i = 0; i < arr->Length(); ++i) {
		v8::Local<v8::Value> v;
		if (!arr->Get(context, i).ToLocal(&v) || !v->IsNumber()) return false;
		out->push_back(static_cast<int32_t>(v.As<v8::Number>()->Value()));
	}
	return true;
}

bool ReadByteArray(v8::Local<v8::Context> context,
			   v8::Local<v8::Value> value,
			   std::vector<uint8_t>* out) {
	if (out == nullptr || !value->IsArray()) return false;
	v8::Local<v8::Array> arr = value.As<v8::Array>();
	out->clear();
	out->reserve(arr->Length());
	for (uint32_t i = 0; i < arr->Length(); ++i) {
		v8::Local<v8::Value> v;
		if (!arr->Get(context, i).ToLocal(&v) || !v->IsNumber()) return false;
		out->push_back(static_cast<uint8_t>(std::clamp<double>(v.As<v8::Number>()->Value(), 0.0, 255.0)));
	}
	return true;
}

v8::Local<v8::Array> MakeFloatArray(v8::Isolate* isolate,
				    v8::Local<v8::Context> context,
				    const std::vector<float>& values) {
	v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(values.size()));
	for (uint32_t i = 0; i < values.size(); ++i) {
		array->Set(context, i, v8::Number::New(isolate, static_cast<double>(values[i]))).FromMaybe(false);
	}
	return array;
}

v8::Local<v8::Array> MakeIntArray(v8::Isolate* isolate,
				  v8::Local<v8::Context> context,
				  const std::vector<int32_t>& values) {
	v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(values.size()));
	for (uint32_t i = 0; i < values.size(); ++i) {
		array->Set(context, i, v8::Integer::New(isolate, values[i])).FromMaybe(false);
	}
	return array;
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
	SetProperty(isolate, context, obj, "elementCount", v8::Number::New(isolate, static_cast<double>(tensor.values.size())));
	if (include_data) {
		SetProperty(isolate, context, obj, "data", MakeFloatArray(isolate, context, tensor.values));
	}
	return obj;
}

bool CopyFloatsToTensor(const std::vector<float>& values, Tensor* tensor) {
	if (tensor == nullptr || tensor->GetData() == nullptr) return false;
	if (values.size() != tensor->GetElementCount()) return false;
	switch (tensor->GetDataType()) {
		case DataType::FLOAT32: {
			auto* data = static_cast<float*>(tensor->GetData());
			std::copy(values.begin(), values.end(), data);
			return true;
		}
		case DataType::FLOAT64: {
			auto* data = static_cast<double*>(tensor->GetData());
			for (size_t i = 0; i < values.size(); ++i) data[i] = static_cast<double>(values[i]);
			return true;
		}
		case DataType::INT32: {
			auto* data = static_cast<int32_t*>(tensor->GetData());
			for (size_t i = 0; i < values.size(); ++i) data[i] = static_cast<int32_t>(values[i]);
			return true;
		}
		case DataType::INT64: {
			auto* data = static_cast<int64_t*>(tensor->GetData());
			for (size_t i = 0; i < values.size(); ++i) data[i] = static_cast<int64_t>(values[i]);
			return true;
		}
		case DataType::UINT32: {
			auto* data = static_cast<uint32_t*>(tensor->GetData());
			for (size_t i = 0; i < values.size(); ++i) data[i] = static_cast<uint32_t>(std::max<float>(0.0f, values[i]));
			return true;
		}
		case DataType::UINT8: {
			auto* data = static_cast<uint8_t*>(tensor->GetData());
			for (size_t i = 0; i < values.size(); ++i) data[i] = static_cast<uint8_t>(std::clamp<float>(values[i], 0.0f, 255.0f));
			return true;
		}
	}
	return false;
}

std::vector<float> CopyTensorToFloats(const Tensor& tensor) {
	std::vector<float> values(static_cast<size_t>(tensor.GetElementCount()), 0.0f);
	if (tensor.GetData() == nullptr) return values;
	switch (tensor.GetDataType()) {
		case DataType::FLOAT32: {
			const auto* data = static_cast<const float*>(tensor.GetData());
			std::copy(data, data + values.size(), values.begin());
			break;
		}
		case DataType::FLOAT64: {
			const auto* data = static_cast<const double*>(tensor.GetData());
			for (size_t i = 0; i < values.size(); ++i) values[i] = static_cast<float>(data[i]);
			break;
		}
		case DataType::INT32: {
			const auto* data = static_cast<const int32_t*>(tensor.GetData());
			for (size_t i = 0; i < values.size(); ++i) values[i] = static_cast<float>(data[i]);
			break;
		}
		case DataType::INT64: {
			const auto* data = static_cast<const int64_t*>(tensor.GetData());
			for (size_t i = 0; i < values.size(); ++i) values[i] = static_cast<float>(data[i]);
			break;
		}
		case DataType::UINT32: {
			const auto* data = static_cast<const uint32_t*>(tensor.GetData());
			for (size_t i = 0; i < values.size(); ++i) values[i] = static_cast<float>(data[i]);
			break;
		}
		case DataType::UINT8: {
			const auto* data = static_cast<const uint8_t*>(tensor.GetData());
			for (size_t i = 0; i < values.size(); ++i) values[i] = static_cast<float>(data[i]);
			break;
		}
	}
	return values;
}

bool ParseTensorArgs(const v8::FunctionCallbackInfo<v8::Value>& args,
			     int shape_index,
			     int data_index,
			     int dtype_index,
			     std::vector<uint32_t>* shape_out,
			     std::vector<float>* data_out,
			     DataType* dtype_out) {
	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	if (!ReadUIntArray(context, args[shape_index], shape_out) || !ReadFloatArray(context, args[data_index], data_out)) {
		return false;
	}
	std::string dtype_text = "float32";
	if (dtype_index >= 0 && args.Length() > dtype_index && args[dtype_index]->IsString()) {
		dtype_text = Engine::Helper::FromV8Str(args.GetIsolate(), args[dtype_index]);
	}
	*dtype_out = ParseTensorDataType(dtype_text);
	uint64_t expected = 1;
	for (uint32_t dim : *shape_out) expected *= static_cast<uint64_t>(std::max<uint32_t>(1, dim));
	return expected == data_out->size();
}

Tensor MakeTensorFromArgs(const std::vector<uint32_t>& shape,
			 const std::vector<float>& data,
			 DataType dtype) {
	Tensor tensor(shape, dtype);
	CopyFloatsToTensor(data, &tensor);
	return tensor;
}

v8::Local<v8::Object> MakeTensorValueObject(v8::Isolate* isolate,
					    v8::Local<v8::Context> context,
					    const Tensor& tensor,
					    const std::string& name) {
	AiTensorData serialized{tensor.GetShape(), CopyTensorToFloats(tensor)};
	v8::Local<v8::Object> obj = MakeTensorObject(isolate, context, name, serialized, true);
	SetProperty(isolate, context, obj, "dtype", Engine::Helper::ToV8Str(isolate, ToTensorDataTypeName(tensor.GetDataType())));
	SetProperty(isolate, context, obj, "memorySize", v8::Number::New(isolate, static_cast<double>(tensor.GetMemorySize())));
	return obj;
}

v8::Local<v8::Object> MakeValidationObject(v8::Isolate* isolate,
					   v8::Local<v8::Context> context,
					   const ValidationResult& result) {
	v8::Local<v8::Object> obj = v8::Object::New(isolate);
	SetProperty(isolate, context, obj, "isValid", v8::Boolean::New(isolate, result.is_valid));
	SetProperty(isolate, context, obj, "errorMessage", Engine::Helper::ToV8Str(isolate, result.error_message));
	SetProperty(isolate, context, obj, "completenessScore", v8::Number::New(isolate, result.completeness_score));
	return obj;
}

v8::Local<v8::Object> MakeDatasetObject(v8::Isolate* isolate,
					v8::Local<v8::Context> context,
					const std::string& handle,
					const std::shared_ptr<Dataset>& dataset,
					const std::string& loader_type,
					const std::string& source_path) {
	v8::Local<v8::Object> obj = v8::Object::New(isolate);
	SetProperty(isolate, context, obj, "handle", Engine::Helper::ToV8Str(isolate, handle));
	SetProperty(isolate, context, obj, "name", Engine::Helper::ToV8Str(isolate, dataset ? dataset->GetDatasetName() : std::string()));
	SetProperty(isolate, context, obj, "batchCount", v8::Number::New(isolate, dataset ? static_cast<double>(dataset->GetBatchCount()) : 0.0));
	SetProperty(isolate, context, obj, "totalSamples", v8::Number::New(isolate, dataset ? static_cast<double>(dataset->GetTotalSamples()) : 0.0));
	SetProperty(isolate, context, obj, "loaderType", Engine::Helper::ToV8Str(isolate, loader_type));
	SetProperty(isolate, context, obj, "sourcePath", Engine::Helper::ToV8Str(isolate, source_path));
	return obj;
}

std::unique_ptr<DataLoader> CreateLoader(v8::Isolate* isolate,
					 v8::Local<v8::Context> context,
					 const std::string& kind,
					 v8::Local<v8::Value> options_value) {
	const std::string lowered = ToLowerCopy(kind);
	if (lowered == "csv") {
		auto loader = std::make_unique<CSVLoader>();
		if (!options_value.IsEmpty() && options_value->IsObject()) {
			v8::Local<v8::Object> options = options_value.As<v8::Object>();
			const std::string delimiter = GetObjectString(isolate, context, options, "delimiter", nullptr, ",");
			if (!delimiter.empty()) loader->SetDelimiter(delimiter.front());
		}
		return loader;
	}
	if (lowered == "image") {
		auto loader = std::make_unique<ImageLoader>();
		if (!options_value.IsEmpty() && options_value->IsObject()) {
			v8::Local<v8::Object> options = options_value.As<v8::Object>();
			v8::Local<v8::Value> width_value;
			v8::Local<v8::Value> height_value;
			if (GetObjectValue(isolate, context, options, "width", nullptr, &width_value) &&
			    GetObjectValue(isolate, context, options, "height", nullptr, &height_value) &&
			    width_value->IsNumber() && height_value->IsNumber()) {
				loader->SetImageSize(static_cast<uint32_t>(std::max<double>(0.0, width_value.As<v8::Number>()->Value())),
						     static_cast<uint32_t>(std::max<double>(0.0, height_value.As<v8::Number>()->Value())));
			}
		}
		return loader;
	}
	return std::make_unique<DataLoader>();
}

std::shared_ptr<Dataset> ResolveDatasetHandle(const std::string& handle) {
	auto it = LoadedDatasets().find(handle);
	return it == LoadedDatasets().end() ? nullptr : it->second;
}

void AiVersionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> obj = v8::Object::New(isolate);
	SetProperty(isolate, context, obj, "version", Engine::Helper::ToV8Str(isolate, MlVersionManager::GetVersion()));
	SetProperty(isolate, context, obj, "buildTag", Engine::Helper::ToV8Str(isolate, MlVersionManager::GetBuildTag()));
	SetProperty(isolate, context, obj, "backendSummary", Engine::Helper::ToV8Str(isolate, MlVersionManager::GetBackendSummary()));
	args.GetReturnValue().Set(obj);
}

void AiMlLogCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string level;
	std::string message;
	if (!RequireStringArg(args, 0, "AI.mlLog expects level", &level) || !RequireStringArg(args, 1, "AI.mlLog expects message", &message)) return;
	MlLogger::GetInstance().Log(ParseMlLogLevel(level), message);
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
}

void AiMlGetLogsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	const auto& entries = MlLogger::GetInstance().GetEntries();
	v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(entries.size()));
	for (uint32_t i = 0; i < entries.size(); ++i) {
		v8::Local<v8::Object> item = v8::Object::New(isolate);
		SetProperty(isolate, context, item, "level", Engine::Helper::ToV8Str(isolate, ToMlLogLevelName(entries[i].level)));
		SetProperty(isolate, context, item, "message", Engine::Helper::ToV8Str(isolate, entries[i].message));
		array->Set(context, i, item).FromMaybe(false);
	}
	args.GetReturnValue().Set(array);
}

void AiMlClearLogsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	MlLogger::GetInstance().Clear();
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
}

void AiMlSetLogLevelCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string level;
	if (!RequireStringArg(args, 0, "AI.mlSetLogLevel expects level", &level)) return;
	MlLogger::GetInstance().SetMinLevel(ParseMlLogLevel(level));
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
}

void AiMathDotCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> a;
	std::vector<float> b;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &a) || !ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[1], &b)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.mathDot expects two float arrays");
		return;
	}
	args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), MlMathUtils::Dot(a, b)));
}

void AiMathMeanCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> values;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &values)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.mathMean expects float array");
		return;
	}
	args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), MlMathUtils::Mean(values)));
}

void AiMathVarianceCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> values;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &values)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.mathVariance expects float array");
		return;
	}
	args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), MlMathUtils::Variance(values)));
}

void AiNormalizeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> values;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &values)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.normalize expects float array");
		return;
	}
	args.GetReturnValue().Set(MakeFloatArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), MlMathUtils::Normalize(values)));
}

void AiSoftmaxCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> values;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &values)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.softmax expects float array");
		return;
	}
	args.GetReturnValue().Set(MakeFloatArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), MlMathUtils::Softmax(values)));
}

void AiL2NormalizeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> values;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &values)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.l2Normalize expects float array");
		return;
	}
	const float epsilon = (args.Length() > 1 && args[1]->IsNumber()) ? static_cast<float>(args[1].As<v8::Number>()->Value()) : 1e-8f;
	args.GetReturnValue().Set(MakeFloatArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), MlMathUtils::L2Normalize(values, epsilon)));
}

void AiRandomFloatCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	const float min_value = (args.Length() > 0 && args[0]->IsNumber()) ? static_cast<float>(args[0].As<v8::Number>()->Value()) : 0.0f;
	const float max_value = (args.Length() > 1 && args[1]->IsNumber()) ? static_cast<float>(args[1].As<v8::Number>()->Value()) : 1.0f;
	args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), GlobalRandom().UniformFloat(min_value, max_value)));
}

void AiRandomIntCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	const int min_value = (args.Length() > 0 && args[0]->IsNumber()) ? args[0].As<v8::Int32>()->Value() : 0;
	const int max_value = (args.Length() > 1 && args[1]->IsNumber()) ? args[1].As<v8::Int32>()->Value() : 100;
	args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), GlobalRandom().UniformInt(min_value, max_value)));
}

void AiBernoulliCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	const float probability = (args.Length() > 0 && args[0]->IsNumber()) ? static_cast<float>(args[0].As<v8::Number>()->Value()) : 0.5f;
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), GlobalRandom().Bernoulli(probability)));
}

void AiSetSeedCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	const uint64_t seed = (args.Length() > 0 && args[0]->IsNumber()) ? static_cast<uint64_t>(std::max<double>(0.0, args[0].As<v8::Number>()->Value())) : 0;
	GlobalRandom().SetSeed(seed);
	args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), static_cast<double>(GlobalRandom().GetSeed())));
}

void AiParseConfigCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string text;
	if (!RequireStringArg(args, 0, "AI.parseConfig expects text", &text)) return;
	MlConfigParser parser;
	const bool ok = parser.ParseText(text);
	v8::Local<v8::Object> result = v8::Object::New(isolate);
	SetProperty(isolate, context, result, "ok", v8::Boolean::New(isolate, ok));
	v8::Local<v8::Object> raw = v8::Object::New(isolate);
	for (const auto& kv : parser.Raw()) {
		SetProperty(isolate, context, raw, kv.first.c_str(), Engine::Helper::ToV8Str(isolate, kv.second));
	}
	SetProperty(isolate, context, result, "raw", raw);
	args.GetReturnValue().Set(result);
}

void AiIntToFloatCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<int32_t> input;
	if (!ReadIntArray(args.GetIsolate()->GetCurrentContext(), args[0], &input)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.intToFloat expects integer array");
		return;
	}
	args.GetReturnValue().Set(MakeFloatArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), MlTypeConverter::IntToFloat(input)));
}

void AiFloatToIntCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> input;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &input)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.floatToInt expects float array");
		return;
	}
	args.GetReturnValue().Set(MakeIntArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), MlTypeConverter::FloatToInt(input)));
}

void AiFloatToBytesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> input;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &input)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.floatToBytes expects float array");
		return;
	}
	args.GetReturnValue().Set(MakeByteArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), MlTypeConverter::FloatToBytes(input)));
}

void AiBytesToFloatCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<uint8_t> input;
	if (!ReadByteArray(args.GetIsolate()->GetCurrentContext(), args[0], &input)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.bytesToFloat expects byte array");
		return;
	}
	args.GetReturnValue().Set(MakeFloatArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), MlTypeConverter::BytesToFloat(input)));
}

void AiFp16RoundTripCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> input;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &input)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.fp16RoundTrip expects float array");
		return;
	}
	args.GetReturnValue().Set(MakeFloatArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), MlTypeConverter::Fp16RoundTrip(input)));
}

void AiProfilerBeginCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string key;
	if (!RequireStringArg(args, 0, "AI.profilerBegin expects key", &key)) return;
	GlobalAiProfiler().Begin(key);
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
}

void AiProfilerEndCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string key;
	if (!RequireStringArg(args, 0, "AI.profilerEnd expects key", &key)) return;
	GlobalAiProfiler().End(key);
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
}

void AiProfilerSnapshotCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	const auto snapshot = GlobalAiProfiler().SnapshotMillis();
	for (const auto& kv : snapshot) {
		SetProperty(isolate, context, object, kv.first.c_str(), v8::Number::New(isolate, kv.second));
	}
	args.GetReturnValue().Set(object);
}

void AiCreateTensorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	if (args.Length() < 2) {
		Engine::Helper::ThrowTypeError(isolate, "AI.createTensor expects shape and data arrays");
		return;
	}
	std::vector<uint32_t> shape;
	std::vector<float> data;
	DataType dtype = DataType::FLOAT32;
	if (!ParseTensorArgs(args, 0, 1, 2, &shape, &data, &dtype)) {
		Engine::Helper::ThrowTypeError(isolate, "invalid tensor shape/data/dtype");
		return;
	}
	Tensor tensor = MakeTensorFromArgs(shape, data, dtype);
	args.GetReturnValue().Set(MakeTensorValueObject(isolate, context, tensor, "tensor"));
}

void AiZeroTensorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::vector<uint32_t> shape;
	if (!ReadUIntArray(context, args[0], &shape)) {
		Engine::Helper::ThrowTypeError(isolate, "AI.zeroTensor expects shape array");
		return;
	}
	std::string dtype_text = "float32";
	if (args.Length() > 1 && args[1]->IsString()) {
		dtype_text = Engine::Helper::FromV8Str(isolate, args[1]);
	}
	Tensor tensor(shape, ParseTensorDataType(dtype_text));
	tensor.Zero();
	args.GetReturnValue().Set(MakeTensorValueObject(isolate, context, tensor, "tensor"));
}

void AiValidateTensorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	if (args.Length() < 2) {
		Engine::Helper::ThrowTypeError(isolate, "AI.validateTensor expects shape and data arrays");
		return;
	}
	std::vector<uint32_t> shape;
	std::vector<float> data;
	DataType dtype = DataType::FLOAT32;
	if (!ParseTensorArgs(args, 0, 1, 2, &shape, &data, &dtype)) {
		Engine::Helper::ThrowTypeError(isolate, "invalid tensor shape/data/dtype");
		return;
	}
	Tensor tensor = MakeTensorFromArgs(shape, data, dtype);
	args.GetReturnValue().Set(MakeValidationObject(isolate, context, DataValidator::GetInstance().ValidateTensor(tensor)));
}

void AiNormalizeTensorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	if (args.Length() < 2) {
		Engine::Helper::ThrowTypeError(isolate, "AI.normalizeTensor expects shape and data arrays");
		return;
	}
	std::vector<uint32_t> shape;
	std::vector<float> data;
	DataType dtype = DataType::FLOAT32;
	if (!ParseTensorArgs(args, 0, 1, 2, &shape, &data, &dtype)) {
		Engine::Helper::ThrowTypeError(isolate, "invalid tensor shape/data/dtype");
		return;
	}
	Normalizer normalizer;
	if (args.Length() > 3 && args[3]->IsNumber()) {
		normalizer.SetMean(static_cast<float>(args[3].As<v8::Number>()->Value()));
	}
	if (args.Length() > 4 && args[4]->IsNumber()) {
		normalizer.SetStdDev(static_cast<float>(args[4].As<v8::Number>()->Value()));
	}
	Tensor tensor = MakeTensorFromArgs(shape, data, dtype);
	std::shared_ptr<Tensor> result = normalizer.Process(tensor);
	args.GetReturnValue().Set(MakeTensorValueObject(isolate, context, *result, "normalized"));
}

void AiAugmentTensorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	if (args.Length() < 3) {
		Engine::Helper::ThrowTypeError(isolate, "AI.augmentTensor expects mode, shape and data");
		return;
	}
	std::string mode;
	if (!RequireStringArg(args, 0, "AI.augmentTensor expects mode", &mode)) return;
	std::vector<uint32_t> shape;
	std::vector<float> data;
	DataType dtype = DataType::FLOAT32;
	if (!ParseTensorArgs(args, 1, 2, 3, &shape, &data, &dtype)) {
		Engine::Helper::ThrowTypeError(isolate, "invalid tensor shape/data/dtype");
		return;
	}
	Tensor tensor = MakeTensorFromArgs(shape, data, dtype);
	AugmentationPipeline pipeline;
	const std::string lowered = ToLowerCopy(mode);
	if (lowered == "rotation") {
		const float angle = (args.Length() > 4 && args[4]->IsNumber()) ? static_cast<float>(args[4].As<v8::Number>()->Value()) : 15.0f;
		pipeline.AddAugmentor(std::make_shared<RotationAugmentor>(angle));
	} else if (lowered == "flip") {
		const float probability = (args.Length() > 4 && args[4]->IsNumber()) ? static_cast<float>(args[4].As<v8::Number>()->Value()) : 0.5f;
		pipeline.AddAugmentor(std::make_shared<FlipAugmentor>(FlipAugmentor::FlipMode::Horizontal, probability));
	} else if (lowered == "crop") {
		const float min_crop = (args.Length() > 4 && args[4]->IsNumber()) ? static_cast<float>(args[4].As<v8::Number>()->Value()) : 0.8f;
		const float max_crop = (args.Length() > 5 && args[5]->IsNumber()) ? static_cast<float>(args[5].As<v8::Number>()->Value()) : 1.0f;
		pipeline.AddAugmentor(std::make_shared<CropAugmentor>(min_crop, max_crop));
	} else if (lowered == "colorjitter" || lowered == "color") {
		const float brightness = (args.Length() > 4 && args[4]->IsNumber()) ? static_cast<float>(args[4].As<v8::Number>()->Value()) : 0.2f;
		const float contrast = (args.Length() > 5 && args[5]->IsNumber()) ? static_cast<float>(args[5].As<v8::Number>()->Value()) : 0.2f;
		const float saturation = (args.Length() > 6 && args[6]->IsNumber()) ? static_cast<float>(args[6].As<v8::Number>()->Value()) : 0.2f;
		pipeline.AddAugmentor(std::make_shared<ColorJitterAugmentor>(brightness, contrast, saturation));
	} else if (lowered == "noise" || lowered == "gaussiannoise") {
		const float stddev = (args.Length() > 4 && args[4]->IsNumber()) ? static_cast<float>(args[4].As<v8::Number>()->Value()) : 0.05f;
		pipeline.AddAugmentor(std::make_shared<NoiseAugmentor>(stddev));
	} else {
		Engine::Helper::ThrowRangeError(isolate, "unsupported augmentation mode");
		return;
	}
	Tensor result = pipeline.Apply(tensor);
	v8::Local<v8::Object> output = MakeTensorValueObject(isolate, context, result, lowered);
	SetProperty(isolate, context, output, "augmentors", v8::Number::New(isolate, static_cast<double>(pipeline.GetAugmentorCount())));
	args.GetReturnValue().Set(output);
}

void AiFeatureDotProductCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> a;
	std::vector<float> b;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &a) || !ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[1], &b)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.featureDotProduct expects two float arrays");
		return;
	}
	args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), FeatMathOps::DotProduct(a, b)));
}

void AiFeatureL2NormCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> values;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &values)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.featureL2Norm expects float array");
		return;
	}
	args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), FeatMathOps::L2Norm(values)));
}

void AiFeatureAddCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> a;
	std::vector<float> b;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &a) || !ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[1], &b)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.featureAdd expects two float arrays");
		return;
	}
	args.GetReturnValue().Set(MakeFloatArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), FeatMathOps::Add(a, b)));
}

void AiFeatureSubtractCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> a;
	std::vector<float> b;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &a) || !ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[1], &b)) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.featureSubtract expects two float arrays");
		return;
	}
	args.GetReturnValue().Set(MakeFloatArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), FeatMathOps::Subtract(a, b)));
}

void AiFeatureScaleCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> values;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &values) || !args[1]->IsNumber()) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.featureScale expects float array and scale");
		return;
	}
	const float scale = static_cast<float>(args[1].As<v8::Number>()->Value());
	args.GetReturnValue().Set(MakeFloatArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), FeatMathOps::Scale(values, scale)));
}

void AiFeatureMatMulCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> a;
	std::vector<float> b;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &a) || !args[1]->IsNumber() || !args[2]->IsNumber() || !ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[3], &b) || !args[4]->IsNumber()) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.featureMatMul expects A, m, k, B, n");
		return;
	}
	const size_t m = static_cast<size_t>(std::max<double>(0.0, args[1].As<v8::Number>()->Value()));
	const size_t k = static_cast<size_t>(std::max<double>(0.0, args[2].As<v8::Number>()->Value()));
	const size_t n = static_cast<size_t>(std::max<double>(0.0, args[4].As<v8::Number>()->Value()));
	args.GetReturnValue().Set(MakeFloatArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), FeatMathOps::MatMul(a, m, k, b, n)));
}

void AiFeatureCovarianceCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::vector<float> x;
	if (!ReadFloatArray(args.GetIsolate()->GetCurrentContext(), args[0], &x) || !args[1]->IsNumber() || !args[2]->IsNumber()) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "AI.featureCovariance expects X, nSamples, nFeatures");
		return;
	}
	const size_t n_samples = static_cast<size_t>(std::max<double>(0.0, args[1].As<v8::Number>()->Value()));
	const size_t n_features = static_cast<size_t>(std::max<double>(0.0, args[2].As<v8::Number>()->Value()));
	args.GetReturnValue().Set(MakeFloatArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), FeatMathOps::CovarianceMatrix(x, n_samples, n_features)));
}

void AiLoadDatasetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string path;
	if (!RequireStringArg(args, 0, "AI.loadDataset expects path", &path)) return;
	std::string kind = "generic";
	v8::Local<v8::Value> options_value = v8::Undefined(isolate);
	if (args.Length() > 1 && args[1]->IsString()) {
		kind = Engine::Helper::FromV8Str(isolate, args[1]);
		if (args.Length() > 2) options_value = args[2];
	} else if (args.Length() > 1) {
		options_value = args[1];
	}
	auto loader = CreateLoader(isolate, context, kind, options_value);
	std::shared_ptr<Dataset> dataset = loader->LoadDataset(path);
	if (!dataset) {
		args.GetReturnValue().Set(v8::Null(isolate));
		return;
	}
	const std::string handle = NextDatasetHandle();
	LoadedDatasets()[handle] = dataset;
	std::string cache_key;
	if (!options_value.IsEmpty() && options_value->IsObject()) {
		cache_key = GetObjectString(isolate, context, options_value.As<v8::Object>(), "cacheKey", nullptr, std::string());
		if (!cache_key.empty()) {
			DataCache::GetInstance().CacheDataset(cache_key, dataset);
		}
	}
	v8::Local<v8::Object> result = MakeDatasetObject(isolate, context, handle, dataset, kind, path);
	if (!cache_key.empty()) {
		SetProperty(isolate, context, result, "cacheKey", Engine::Helper::ToV8Str(isolate, cache_key));
	}
	args.GetReturnValue().Set(result);
}

void AiGetDatasetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string handle;
	if (!RequireStringArg(args, 0, "AI.getDataset expects handle", &handle)) return;
	std::shared_ptr<Dataset> dataset = ResolveDatasetHandle(handle);
	if (!dataset) {
		args.GetReturnValue().Set(v8::Null(isolate));
		return;
	}
	args.GetReturnValue().Set(MakeDatasetObject(isolate, context, handle, dataset, "registry", std::string()));
}

void AiListDatasetsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(LoadedDatasets().size()));
	uint32_t index = 0;
	for (const auto& kv : LoadedDatasets()) {
		array->Set(context, index++, MakeDatasetObject(isolate, context, kv.first, kv.second, "registry", std::string())).FromMaybe(false);
	}
	args.GetReturnValue().Set(array);
}

void AiDropDatasetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string handle;
	if (!RequireStringArg(args, 0, "AI.dropDataset expects handle", &handle)) return;
	const bool erased = LoadedDatasets().erase(handle) > 0;
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), erased));
}

void AiSaveDatasetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string path;
	std::string handle;
	if (!RequireStringArg(args, 0, "AI.saveDataset expects path", &path) || !RequireStringArg(args, 1, "AI.saveDataset expects handle", &handle)) return;
	std::string kind = "generic";
	v8::Local<v8::Value> options_value = v8::Undefined(isolate);
	if (args.Length() > 2 && args[2]->IsString()) {
		kind = Engine::Helper::FromV8Str(isolate, args[2]);
		if (args.Length() > 3) options_value = args[3];
	} else if (args.Length() > 2) {
		options_value = args[2];
	}
	std::shared_ptr<Dataset> dataset = ResolveDatasetHandle(handle);
	if (!dataset) {
		Engine::Helper::ThrowRangeError(isolate, "dataset handle not found");
		return;
	}
	auto loader = CreateLoader(isolate, context, kind, options_value);
	args.GetReturnValue().Set(v8::Boolean::New(isolate, loader->SaveDataset(path, *dataset)));
}

void AiCacheDatasetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string key;
	std::string handle;
	if (!RequireStringArg(args, 0, "AI.cacheDataset expects key", &key) || !RequireStringArg(args, 1, "AI.cacheDataset expects handle", &handle)) return;
	std::shared_ptr<Dataset> dataset = ResolveDatasetHandle(handle);
	if (!dataset) {
		Engine::Helper::ThrowRangeError(args.GetIsolate(), "dataset handle not found");
		return;
	}
	DataCache::GetInstance().CacheDataset(key, dataset);
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
}

void AiGetCachedDatasetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string key;
	if (!RequireStringArg(args, 0, "AI.getCachedDataset expects key", &key)) return;
	std::shared_ptr<Dataset> dataset = DataCache::GetInstance().GetCachedDataset(key);
	if (!dataset) {
		args.GetReturnValue().Set(v8::Null(isolate));
		return;
	}
	args.GetReturnValue().Set(MakeDatasetObject(isolate, context, key, dataset, "cache", std::string()));
}

void AiIsDatasetCachedCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string key;
	if (!RequireStringArg(args, 0, "AI.isDatasetCached expects key", &key)) return;
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), DataCache::GetInstance().IsCached(key)));
}

void AiRemoveCachedDatasetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string key;
	if (!RequireStringArg(args, 0, "AI.removeCachedDataset expects key", &key)) return;
	DataCache::GetInstance().RemoveEntry(key);
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
}

void AiClearDatasetCacheCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	DataCache::GetInstance().ClearCache();
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
}

void AiGetDatasetCacheSizeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), static_cast<double>(DataCache::GetInstance().GetCacheSize())));
}

}  // namespace

bool RegisterAiMlMethods(v8::Isolate* isolate,
				 v8::Local<v8::Context> context,
				 v8::Local<v8::Object> module,
				 std::string* error_out) {
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "version", &AiVersionCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "mlLog", &AiMlLogCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "mlGetLogs", &AiMlGetLogsCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "mlClearLogs", &AiMlClearLogsCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "mlSetLogLevel", &AiMlSetLogLevelCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "mathDot", &AiMathDotCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "mathMean", &AiMathMeanCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "mathVariance", &AiMathVarianceCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "normalize", &AiNormalizeCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "softmax", &AiSoftmaxCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "l2Normalize", &AiL2NormalizeCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "randomFloat", &AiRandomFloatCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "randomInt", &AiRandomIntCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "bernoulli", &AiBernoulliCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "setSeed", &AiSetSeedCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "parseConfig", &AiParseConfigCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "intToFloat", &AiIntToFloatCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "floatToInt", &AiFloatToIntCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "floatToBytes", &AiFloatToBytesCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "bytesToFloat", &AiBytesToFloatCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "fp16RoundTrip", &AiFp16RoundTripCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "profilerBegin", &AiProfilerBeginCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "profilerEnd", &AiProfilerEndCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "profilerSnapshot", &AiProfilerSnapshotCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "createTensor", &AiCreateTensorCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "zeroTensor", &AiZeroTensorCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "validateTensor", &AiValidateTensorCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "normalizeTensor", &AiNormalizeTensorCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "augmentTensor", &AiAugmentTensorCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "featureDotProduct", &AiFeatureDotProductCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "featureL2Norm", &AiFeatureL2NormCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "featureAdd", &AiFeatureAddCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "featureSubtract", &AiFeatureSubtractCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "featureScale", &AiFeatureScaleCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "featureMatMul", &AiFeatureMatMulCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "featureCovariance", &AiFeatureCovarianceCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "loadDataset", &AiLoadDatasetCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "getDataset", &AiGetDatasetCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "listDatasets", &AiListDatasetsCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "dropDataset", &AiDropDatasetCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "saveDataset", &AiSaveDatasetCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "cacheDataset", &AiCacheDatasetCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "getCachedDataset", &AiGetCachedDatasetCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "isDatasetCached", &AiIsDatasetCachedCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "removeCachedDataset", &AiRemoveCachedDatasetCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "clearDatasetCache", &AiClearDatasetCacheCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "getDatasetCacheSize", &AiGetDatasetCacheSizeCallback);
	if (!ok && error_out != nullptr) {
		*error_out = "failed to register AI ML methods";
	}
	return ok;
}

}  // namespace modules::detail