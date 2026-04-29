#include "modules/module_builders.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <unistd.h>

namespace modules::detail {
namespace {

std::string DetectPlatformName() {
#if defined(_WIN32)
	return "windows";
#elif defined(__APPLE__)
	return "macos";
#elif defined(__linux__)
	return "linux";
#elif defined(__OpenBSD__)
	return "openbsd";
#else
	return "unknown";
#endif
}

std::string DetectHostName() {
	std::array<char, 256> hostname {};
	if (gethostname(hostname.data(), hostname.size() - 1u) != 0) {
		return std::string();
	}
	hostname.back() = '\0';
	return hostname.data();
}

std::string WhichCommand(const std::string& command) {
	const char* path_env = std::getenv("PATH");
	if (path_env == nullptr || command.empty()) {
		return std::string();
	}
	std::stringstream stream(path_env);
	std::string segment;
	while (std::getline(stream, segment, ':')) {
		const std::filesystem::path candidate = std::filesystem::path(segment) / command;
		std::error_code error;
		if (std::filesystem::exists(candidate, error) && access(candidate.c_str(), X_OK) == 0) {
			return candidate.string();
		}
	}
	return std::string();
}

void SystemCwdCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::error_code error;
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), std::filesystem::current_path(error).string()));
}

void SystemProjectRootCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), ResolveProjectRoot().string()));
}

void SystemPlatformCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), DetectPlatformName()));
}

void SystemHostnameCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), DetectHostName()));
}

void SystemGetEnvCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string key;
	if (!RequireStringArg(args, 0, "getEnv expects environment variable name", &key)) {
		return;
	}
	const char* value = std::getenv(key.c_str());
	if (value != nullptr) {
		args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), value));
		return;
	}
	if (args.Length() > 1) {
		args.GetReturnValue().Set(args[1]);
		return;
	}
	args.GetReturnValue().Set(v8::Undefined(args.GetIsolate()));
}

void SystemSetEnvCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string key;
	std::string value;
	if (!RequireStringArg(args, 0, "setEnv expects environment variable name", &key)
		|| !RequireStringArg(args, 1, "setEnv expects environment variable value", &value)) {
		return;
	}
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), setenv(key.c_str(), value.c_str(), 1) == 0));
}

void SystemWhichCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string command;
	if (!RequireStringArg(args, 0, "which expects command name", &command)) {
		return;
	}
	const std::string resolved = WhichCommand(command);
	if (resolved.empty()) {
		args.GetReturnValue().Set(v8::Undefined(args.GetIsolate()));
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), resolved));
}

}  // namespace

bool BuildSystemModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "cwd", &SystemCwdCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "projectRoot", &SystemProjectRootCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "platform", &SystemPlatformCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "hostname", &SystemHostnameCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "getEnv", &SystemGetEnvCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "setEnv", &SystemSetEnvCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "which", &SystemWhichCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build System module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail