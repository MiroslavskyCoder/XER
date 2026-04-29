#include "modules/module_builders.h"

#include <filesystem>
#include <string>

namespace modules::detail {
namespace {

void UtilNormalizePathCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "normalizePath expects path string", &path)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), std::filesystem::path(path).lexically_normal().generic_string()));
}

void UtilJoinPathCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::filesystem::path joined;
	for (int index = 0; index < args.Length(); ++index) {
		if (!args[index]->IsString()) {
			Engine::Helper::ThrowTypeError(args.GetIsolate(), "joinPath expects string segments");
			return;
		}
		joined /= std::filesystem::path(Engine::Helper::FromV8Str(args.GetIsolate(), args[index]));
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), joined.lexically_normal().generic_string()));
}

void UtilBasenameCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "basename expects path string", &path)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), std::filesystem::path(path).filename().string()));
}

void UtilDirnameCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "dirname expects path string", &path)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), std::filesystem::path(path).parent_path().generic_string()));
}

void UtilExtensionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "extension expects path string", &path)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), std::filesystem::path(path).extension().string()));
}

void UtilCurrentDirCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::error_code error;
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), std::filesystem::current_path(error).string()));
}

void UtilSourceRootCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), ResolveSourceRoot().string()));
}

}  // namespace

bool BuildUtilModule(v8::Isolate* isolate,
			 v8::Local<v8::Context> context,
			 v8::Local<v8::Object>* module_out,
			 std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "normalizePath", &UtilNormalizePathCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "joinPath", &UtilJoinPathCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "basename", &UtilBasenameCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "dirname", &UtilDirnameCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "extension", &UtilExtensionCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "cwd", &UtilCurrentDirCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "sourceRoot", &UtilSourceRootCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Util module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail