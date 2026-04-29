#include "modules/module_builders.h"

#include "async_io/async_file_reader.h"
#include "helper/tool_to.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace modules::detail {
namespace {

std::string BytesToHex(const uint8_t* bytes, size_t size) {
	static constexpr char kHex[] = "0123456789abcdef";
	std::string out;
	out.resize(size * 2u);
	for (size_t index = 0; index < size; ++index) {
		out[index * 2u] = kHex[(bytes[index] >> 4u) & 0x0Fu];
		out[index * 2u + 1u] = kHex[bytes[index] & 0x0Fu];
	}
	return out;
}

void IOAsyncReadTextCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	std::string path;
	if (!RequireStringArg(args, 0, "readText expects path string", &path)) {
		return;
	}
	std::vector<uint8_t> bytes;
	std::string error;
	if (!ReadAllBytes(std::filesystem::path(path), &bytes, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(isolate, std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size())));
}

void IOAsyncReadHexCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	std::string path;
	if (!RequireStringArg(args, 0, "readHex expects path string", &path)) {
		return;
	}
	std::vector<uint8_t> bytes;
	std::string error;
	if (!ReadAllBytes(std::filesystem::path(path), &bytes, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(isolate, BytesToHex(bytes.data(), bytes.size())));
}

void IOAsyncWriteTextCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	std::string path;
	std::string text;
	if (!RequireStringArg(args, 0, "writeText expects path string", &path)
		|| !RequireStringArg(args, 1, "writeText expects content string", &text)) {
		return;
	}
	const bool append = args.Length() > 2 && args[2]->BooleanValue(isolate);
	std::vector<uint8_t> bytes(text.begin(), text.end());
	std::string error;
	if (!WriteAllBytes(std::filesystem::path(path), bytes, append, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	args.GetReturnValue().Set(v8::Boolean::New(isolate, true));
}

void IOAsyncCopyBinaryCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	std::string source;
	std::string destination;
	if (!RequireStringArg(args, 0, "copyBinary expects source path string", &source)
		|| !RequireStringArg(args, 1, "copyBinary expects destination path string", &destination)) {
		return;
	}
	std::string error;
	const bool ok = ToolTo::CopyBinary(std::filesystem::path(source), std::filesystem::path(destination), &error);
	if (!ok && !error.empty()) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void IOAsyncFileSizeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	std::string path;
	if (!RequireStringArg(args, 0, "fileSize expects path string", &path)) {
		return;
	}
	IO::AsyncIO::AsyncFileReader reader;
	if (!reader.OpenFile(path)) {
		Engine::Helper::ThrowError(isolate, reader.GetLastError());
		return;
	}
	const int64_t size = reader.GetFileSize();
	reader.CloseFile();
	args.GetReturnValue().Set(v8::Number::New(isolate, static_cast<double>(size)));
}

}  // namespace

bool BuildIOAsyncModule(v8::Isolate* isolate,
			    v8::Local<v8::Context> context,
			    v8::Local<v8::Object>* module_out,
			    std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "readText", &IOAsyncReadTextCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "readHex", &IOAsyncReadHexCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "writeText", &IOAsyncWriteTextCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "copyBinary", &IOAsyncCopyBinaryCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "fileSize", &IOAsyncFileSizeCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build IOAsync module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail