#include "modules/module_builders.h"

#include "image/io/image_saver.h"
#include "modules/module_common.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace modules::detail {
namespace {

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

v8::Local<v8::Array> MakeByteArray(v8::Isolate* isolate,
				   v8::Local<v8::Context> context,
				   const std::vector<uint8_t>& values) {
	v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(values.size()));
	for (uint32_t i = 0; i < values.size(); ++i) {
		array->Set(context, i, v8::Integer::New(isolate, static_cast<int>(values[i]))).FromMaybe(false);
	}
	return array;
}

bool ParseImageObject(v8::Isolate* isolate,
		      v8::Local<v8::Context> context,
		      v8::Local<v8::Object> object,
		      image::ImageBuffer* out,
		      std::string* error_out) {
	if (out == nullptr) return false;
	v8::Local<v8::Value> width_value;
	v8::Local<v8::Value> height_value;
	v8::Local<v8::Value> data_value;
	if (!GetObjectValue(isolate, context, object, "width", nullptr, &width_value) || !width_value->IsNumber() ||
	    !GetObjectValue(isolate, context, object, "height", nullptr, &height_value) || !height_value->IsNumber() ||
	    !GetObjectValue(isolate, context, object, "data", nullptr, &data_value)) {
		if (error_out != nullptr) *error_out = "Image object must have width, height, data";
		return false;
	}

	const int width = static_cast<int>(std::max<double>(1.0, width_value.As<v8::Number>()->Value()));
	const int height = static_cast<int>(std::max<double>(1.0, height_value.As<v8::Number>()->Value()));
	const std::string pixel_format = GetObjectString(isolate, context, object, "pixelFormat", "pixel_format", "rgba");
	if (ToLowerCopy(pixel_format) != "rgba") {
		if (error_out != nullptr) *error_out = "only RGBA pixelFormat is supported";
		return false;
	}

	std::vector<uint8_t> bytes;
	if (!ReadByteArray(context, data_value, &bytes)) {
		if (error_out != nullptr) *error_out = "Image.data must be an array of bytes";
		return false;
	}
	const size_t expected = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
	if (bytes.size() != expected) {
		if (error_out != nullptr) {
			*error_out = "Image.data size mismatch, expected " + std::to_string(expected) + " bytes";
		}
		return false;
	}

	out->Allocate(width, height, image::PixelFormat::RGBA8, image::ColorSpace::sRGB);
	if (!out->IsValid()) {
		if (error_out != nullptr) *error_out = "failed to allocate ImageBuffer";
		return false;
	}
	std::copy(bytes.begin(), bytes.end(), out->Data());
	return true;
}

void ImageInfoCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "name", Engine::Helper::ToV8Str(isolate, "Image"));
	SetProperty(isolate, context, object, "version", Engine::Helper::ToV8Str(isolate, "image/1"));
	SetProperty(isolate, context, object, "pixelFormat", Engine::Helper::ToV8Str(isolate, "rgba"));
	args.GetReturnValue().Set(object);
}

void ImageCreateRgbaCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	if (args.Length() < 3 || !args[0]->IsNumber() || !args[1]->IsNumber()) {
		Engine::Helper::ThrowTypeError(isolate, "Image.createRgba expects width, height, data[]");
		return;
	}
	const uint32_t width = static_cast<uint32_t>(std::max<double>(1.0, args[0].As<v8::Number>()->Value()));
	const uint32_t height = static_cast<uint32_t>(std::max<double>(1.0, args[1].As<v8::Number>()->Value()));
	std::vector<uint8_t> bytes;
	if (!ReadByteArray(context, args[2], &bytes)) {
		Engine::Helper::ThrowTypeError(isolate, "Image.createRgba expects data array");
		return;
	}
	const size_t expected = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
	if (bytes.size() != expected) {
		Engine::Helper::ThrowRangeError(isolate, "RGBA byte array size mismatch");
		return;
	}

	v8::Local<v8::Object> obj = v8::Object::New(isolate);
	SetProperty(isolate, context, obj, "width", v8::Number::New(isolate, static_cast<double>(width)));
	SetProperty(isolate, context, obj, "height", v8::Number::New(isolate, static_cast<double>(height)));
	SetProperty(isolate, context, obj, "pixelFormat", Engine::Helper::ToV8Str(isolate, "rgba"));
	SetProperty(isolate, context, obj, "data", MakeByteArray(isolate, context, bytes));
	args.GetReturnValue().Set(obj);
}

void ImageSaveCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string path;
	if (!RequireStringArg(args, 0, "Image.save expects path", &path) || args.Length() < 2 || !args[1]->IsObject()) {
		return;
	}
	int quality = 90;
	if (args.Length() > 2 && args[2]->IsNumber()) {
		quality = static_cast<int>(std::clamp<double>(args[2].As<v8::Number>()->Value(), 1.0, 100.0));
	}

	image::ImageBuffer image_buffer;
	std::string parse_error;
	if (!ParseImageObject(isolate, context, args[1].As<v8::Object>(), &image_buffer, &parse_error)) {
		Engine::Helper::ThrowTypeError(isolate, parse_error.c_str());
		return;
	}

	image::ImageSaver saver;
	args.GetReturnValue().Set(v8::Boolean::New(isolate, saver.Save(image_buffer, path, quality)));
}

void ImageSaveRgbaCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string path;
	if (!RequireStringArg(args, 0, "Image.saveRgba expects path", &path) || args.Length() < 4) {
		return;
	}
	if (!args[1]->IsNumber() || !args[2]->IsNumber()) {
		Engine::Helper::ThrowTypeError(isolate, "Image.saveRgba expects width and height numbers");
		return;
	}
	const int width = static_cast<int>(std::max<double>(1.0, args[1].As<v8::Number>()->Value()));
	const int height = static_cast<int>(std::max<double>(1.0, args[2].As<v8::Number>()->Value()));
	std::vector<uint8_t> bytes;
	if (!ReadByteArray(context, args[3], &bytes)) {
		Engine::Helper::ThrowTypeError(isolate, "Image.saveRgba expects data array");
		return;
	}
	const size_t expected = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
	if (bytes.size() != expected) {
		Engine::Helper::ThrowRangeError(isolate, "RGBA byte array size mismatch");
		return;
	}
	int quality = 90;
	if (args.Length() > 4 && args[4]->IsNumber()) {
		quality = static_cast<int>(std::clamp<double>(args[4].As<v8::Number>()->Value(), 1.0, 100.0));
	}

	image::ImageBuffer image_buffer(width, height, image::PixelFormat::RGBA8, image::ColorSpace::sRGB);
	std::copy(bytes.begin(), bytes.end(), image_buffer.Data());
	image::ImageSaver saver;
	args.GetReturnValue().Set(v8::Boolean::New(isolate, saver.Save(image_buffer, path, quality)));
}

}  // namespace

bool BuildImageModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  v8::Local<v8::Object>* module_out,
			  std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = SetProperty(isolate, context, module, "name", Engine::Helper::ToV8Str(isolate, "Image"));
	ok = ok && SetProperty(isolate, context, module, "available", v8::Boolean::New(isolate, true));
	ok = ok && SetProperty(isolate, context, module, "summary",
			       Engine::Helper::ToV8Str(isolate, "Image bridge for RGBA objects and file saving"));
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "info", &ImageInfoCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "createRgba", &ImageCreateRgbaCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "save", &ImageSaveCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "saveRgba", &ImageSaveRgbaCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Image module";
		}
		return false;
	}

	*module_out = module;
	return true;
}

}  // namespace modules::detail
