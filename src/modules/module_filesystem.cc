#include "modules/module_builders.h"

#include "helper/tool_to.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace modules::detail {
namespace {

void FileSystemExistsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "exists expects path string", &path)) {
		return;
	}
	std::error_code error;
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), std::filesystem::exists(std::filesystem::path(path), error)));
}

void FileSystemIsFileCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "isFile expects path string", &path)) {
		return;
	}
	std::error_code error;
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), std::filesystem::is_regular_file(std::filesystem::path(path), error)));
}

void FileSystemIsDirectoryCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "isDirectory expects path string", &path)) {
		return;
	}
	std::error_code error;
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), std::filesystem::is_directory(std::filesystem::path(path), error)));
}

void FileSystemReadTextCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "readText expects path string", &path)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), ToolTo::ReadTextFile(std::filesystem::path(path))));
}

void FileSystemWriteTextCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	std::string path;
	std::string text;
	if (!RequireStringArg(args, 0, "writeText expects path string", &path)
		|| !RequireStringArg(args, 1, "writeText expects content string", &text)) {
		return;
	}
	std::error_code dir_error;
	const std::filesystem::path output_path(path);
	if (!output_path.parent_path().empty()) {
		std::filesystem::create_directories(output_path.parent_path(), dir_error);
	}
	std::string write_error;
	const bool ok = ToolTo::WriteTextFile(output_path, text, false, &write_error);
	if (!ok && !write_error.empty()) {
		Engine::Helper::ThrowError(isolate, write_error);
		return;
	}
	args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void FileSystemCreateDirectoriesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "createDirectories expects path string", &path)) {
		return;
	}
	std::error_code error;
	const bool ok = std::filesystem::create_directories(std::filesystem::path(path), error)
		|| std::filesystem::exists(std::filesystem::path(path), error);
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok && !error));
}

void FileSystemListDirCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string path = OptionalStringArg(args, 0, ".");
	std::vector<std::string> entries;
	std::error_code error;
	if (std::filesystem::exists(std::filesystem::path(path), error) && std::filesystem::is_directory(std::filesystem::path(path), error)) {
		for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(path), std::filesystem::directory_options::skip_permission_denied, error)) {
			if (error) {
				error.clear();
				continue;
			}
			entries.push_back(entry.path().filename().string());
		}
		std::sort(entries.begin(), entries.end());
	}
	args.GetReturnValue().Set(MakeStringArray(isolate, context, entries));
}

}  // namespace

bool BuildFileSystemModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "exists", &FileSystemExistsCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "isFile", &FileSystemIsFileCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "isDirectory", &FileSystemIsDirectoryCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "readText", &FileSystemReadTextCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "writeText", &FileSystemWriteTextCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "createDirectories", &FileSystemCreateDirectoriesCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "listDir", &FileSystemListDirCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build FileSystem module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail