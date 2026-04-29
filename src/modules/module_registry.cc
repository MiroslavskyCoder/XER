#include "modules/module_registry.h"

#include "compiler_source.h"
#include "helper/class_builder.h"
#include "helper/module_builder.h"
#include "helper/tool_to.h"
#include "provider.h"
#include "runtime_live.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace modules {
namespace {

std::string NormalizeModuleKey(const std::string& text) {
	std::string out;
	out.reserve(text.size());
	for (unsigned char ch : text) {
		if (std::isalnum(ch) != 0) {
			out.push_back(static_cast<char>(std::tolower(ch)));
		}
	}
	return out;
}

std::string ToLowerCopy(const std::string& text) {
	std::string out = text;
	std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return out;
}

std::filesystem::path ResolveSourceRoot() {
	const char* env_root = std::getenv("ENGINE_MODULE_ROOT");
	if (env_root != nullptr && env_root[0] != '\0') {
		std::filesystem::path configured_root(env_root);
		std::error_code configured_error;
		if (std::filesystem::exists(configured_root, configured_error)) {
			if (configured_root.filename() == "src") {
				return configured_root.lexically_normal();
			}
			const std::filesystem::path nested_src = configured_root / "src";
			if (std::filesystem::exists(nested_src, configured_error)) {
				return nested_src.lexically_normal();
			}
		}
	}

	std::error_code current_error;
	const std::filesystem::path current = std::filesystem::current_path(current_error);
	if (!current_error) {
		if (current.filename() == "src") {
			return current.lexically_normal();
		}
		const std::filesystem::path nested_src = current / "src";
		std::error_code nested_error;
		if (std::filesystem::exists(nested_src, nested_error)) {
			return nested_src.lexically_normal();
		}
	}

	std::filesystem::path compiled_path = std::filesystem::path(__FILE__).parent_path().parent_path();
	if (compiled_path.filename() == "src") {
		return compiled_path.lexically_normal();
	}
	if (compiled_path.filename() == "modules") {
		return compiled_path.parent_path().lexically_normal();
	}
	return (compiled_path / "src").lexically_normal();
}

bool ResolveRelativeToSourceRoot(const std::string& requested,
					 std::filesystem::path* relative_out,
					 std::filesystem::path* absolute_out,
					 std::string* error_out) {
	if (relative_out == nullptr || absolute_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "source path output target is null";
		}
		return false;
	}

	std::filesystem::path relative = requested.empty()
		? std::filesystem::path()
		: std::filesystem::path(requested).lexically_normal();
	if (relative.is_absolute()) {
		if (error_out != nullptr) {
			*error_out = "source paths must be relative to src";
		}
		return false;
	}
	for (const auto& part : relative) {
		if (part == "..") {
			if (error_out != nullptr) {
				*error_out = "source path escapes src root";
			}
			return false;
		}
	}

	const std::filesystem::path source_root = ResolveSourceRoot();
	*relative_out = relative;
	*absolute_out = source_root / relative;
	return true;
}

template <typename T>
bool SetProperty(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object> object,
			   const char* name,
			   T value) {
	return object->Set(context, Engine::Helper::ToV8Str(isolate, name), value).FromMaybe(false);
}

v8::Local<v8::Array> MakeStringArray(v8::Isolate* isolate,
					 v8::Local<v8::Context> context,
					 const std::vector<std::string>& values) {
	v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(values.size()));
	for (size_t index = 0; index < values.size(); ++index) {
		array
			->Set(context,
			      static_cast<uint32_t>(index),
			      Engine::Helper::ToV8Str(isolate, values[index]))
			.FromMaybe(false);
	}
	return array;
}

bool RequireStringArg(const v8::FunctionCallbackInfo<v8::Value>& args,
			      int index,
			      const char* name,
			      std::string* out) {
	if (out == nullptr) {
		Engine::Helper::ThrowError(args.GetIsolate(), "string output target is null");
		return false;
	}
	if (args.Length() <= index || !args[index]->IsString()) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), name);
		return false;
	}
	*out = Engine::Helper::FromV8Str(args.GetIsolate(), args[index]);
	return true;
}

std::string OptionalStringArg(const v8::FunctionCallbackInfo<v8::Value>& args,
			     int index,
			     const std::string& fallback = std::string()) {
	if (args.Length() <= index || args[index]->IsUndefined() || args[index]->IsNull()) {
		return fallback;
	}
	if (!args[index]->IsString()) {
		return fallback;
	}
	return Engine::Helper::FromV8Str(args.GetIsolate(), args[index]);
}

std::string GetObjectString(v8::Isolate* isolate,
			    v8::Local<v8::Context> context,
			    v8::Local<v8::Object> object,
			    const char* primary_key,
			    const char* secondary_key,
			    const std::string& fallback) {
	auto get_value = [&](const char* key, std::string* out) -> bool {
		if (key == nullptr) {
			return false;
		}
		v8::Local<v8::Value> value;
		if (!object->Get(context, Engine::Helper::ToV8Str(isolate, key)).ToLocal(&value)) {
			return false;
		}
		if (!value->IsString()) {
			return false;
		}
		*out = Engine::Helper::FromV8Str(isolate, value);
		return true;
	};

	std::string out;
	if (get_value(primary_key, &out) || get_value(secondary_key, &out)) {
		return out;
	}
	return fallback;
}

std::vector<std::string> CollectRelativeFiles(const std::filesystem::path& absolute_root,
					      const std::filesystem::path& source_root) {
	std::vector<std::string> files;
	std::error_code error;
	if (!std::filesystem::exists(absolute_root, error)) {
		return files;
	}
	if (std::filesystem::is_regular_file(absolute_root, error)) {
		files.push_back(std::filesystem::relative(absolute_root, source_root, error).generic_string());
		return files;
	}
	std::filesystem::recursive_directory_iterator it(
		absolute_root,
		std::filesystem::directory_options::skip_permission_denied,
		error);
	std::filesystem::recursive_directory_iterator end;
	for (; it != end; it.increment(error)) {
		if (error) {
			error.clear();
			continue;
		}
		if (it->is_regular_file(error)) {
			files.push_back(std::filesystem::relative(it->path(), source_root, error).generic_string());
		}
	}
	std::sort(files.begin(), files.end());
	return files;
}

std::vector<std::string> CollectRelativeDirectories(const std::filesystem::path& absolute_root,
						    const std::filesystem::path& source_root) {
	std::vector<std::string> directories;
	std::error_code error;
	if (!std::filesystem::exists(absolute_root, error) || !std::filesystem::is_directory(absolute_root, error)) {
		return directories;
	}
	std::filesystem::recursive_directory_iterator it(
		absolute_root,
		std::filesystem::directory_options::skip_permission_denied,
		error);
	std::filesystem::recursive_directory_iterator end;
	for (; it != end; it.increment(error)) {
		if (error) {
			error.clear();
			continue;
		}
		if (it->is_directory(error)) {
			directories.push_back(std::filesystem::relative(it->path(), source_root, error).generic_string());
		}
	}
	std::sort(directories.begin(), directories.end());
	return directories;
}

void ContainerSourceRootCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), ResolveSourceRoot().string()));
}

void ContainerListSourceFilesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string error;
	std::filesystem::path relative;
	std::filesystem::path absolute;
	if (!ResolveRelativeToSourceRoot(OptionalStringArg(args, 0), &relative, &absolute, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	args.GetReturnValue().Set(MakeStringArray(isolate, context, CollectRelativeFiles(absolute, ResolveSourceRoot())));
}

void ContainerListSourceDirectoriesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string error;
	std::filesystem::path relative;
	std::filesystem::path absolute;
	if (!ResolveRelativeToSourceRoot(OptionalStringArg(args, 0), &relative, &absolute, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	args.GetReturnValue().Set(MakeStringArray(isolate, context, CollectRelativeDirectories(absolute, ResolveSourceRoot())));
}

void ContainerReadSourceFileCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	std::string requested;
	if (!RequireStringArg(args, 0, "readSourceFile expects src-relative path string", &requested)) {
		return;
	}
	std::string error;
	std::filesystem::path relative;
	std::filesystem::path absolute;
	if (!ResolveRelativeToSourceRoot(requested, &relative, &absolute, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	std::error_code stat_error;
	if (!std::filesystem::exists(absolute, stat_error) || !std::filesystem::is_regular_file(absolute, stat_error)) {
		Engine::Helper::ThrowError(isolate, "source file does not exist");
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(isolate, ToolTo::ReadTextFile(absolute)));
}

void ContainerFindSourceFilesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string query;
	if (!RequireStringArg(args, 0, "findSourceFiles expects search string", &query)) {
		return;
	}
	const std::string lowered_query = ToLowerCopy(query);
	std::vector<std::string> matches;
	for (const auto& file : CollectRelativeFiles(ResolveSourceRoot(), ResolveSourceRoot())) {
		if (ToLowerCopy(file).find(lowered_query) != std::string::npos) {
			matches.push_back(file);
		}
	}
	args.GetReturnValue().Set(MakeStringArray(isolate, context, matches));
}

void ContainerDescribeSourceTreeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string error;
	std::filesystem::path relative;
	std::filesystem::path absolute;
	if (!ResolveRelativeToSourceRoot(OptionalStringArg(args, 0), &relative, &absolute, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}

	const std::filesystem::path source_root = ResolveSourceRoot();
	const std::vector<std::string> files = CollectRelativeFiles(absolute, source_root);
	const std::vector<std::string> directories = CollectRelativeDirectories(absolute, source_root);
	std::vector<std::string> top_level_entries;
	std::error_code iter_error;
	if (std::filesystem::exists(absolute, iter_error) && std::filesystem::is_directory(absolute, iter_error)) {
		for (const auto& entry : std::filesystem::directory_iterator(absolute, std::filesystem::directory_options::skip_permission_denied, iter_error)) {
			if (iter_error) {
				iter_error.clear();
				continue;
			}
			top_level_entries.push_back(std::filesystem::relative(entry.path(), source_root, iter_error).generic_string());
		}
		std::sort(top_level_entries.begin(), top_level_entries.end());
	}

	v8::Local<v8::Object> result = v8::Object::New(isolate);
	SetProperty(isolate, context, result, "sourceRoot", Engine::Helper::ToV8Str(isolate, source_root.string()));
	SetProperty(isolate, context, result, "basePath", Engine::Helper::ToV8Str(isolate, relative.empty() ? std::string(".") : relative.generic_string()));
	SetProperty(isolate, context, result, "fileCount", v8::Number::New(isolate, static_cast<double>(files.size())));
	SetProperty(isolate, context, result, "directoryCount", v8::Number::New(isolate, static_cast<double>(directories.size())));
	SetProperty(isolate, context, result, "topLevelEntries", MakeStringArray(isolate, context, top_level_entries));
	args.GetReturnValue().Set(result);
}

void ContainerAvailableModulesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	args.GetReturnValue().Set(MakeStringArray(isolate, context, ListModules()));
}

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

Provider* UnwrapProviderStore(const v8::FunctionCallbackInfo<v8::Value>& args) {
	if (!args.This()->IsObject()) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "ProviderStore receiver is invalid");
		return nullptr;
	}
	Provider* provider = Engine::Helper::UnwrapPointer<Provider>(args.This().As<v8::Object>());
	if (provider == nullptr) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "ProviderStore is not initialized");
	}
	return provider;
}

void PopulateProviderFromObject(v8::Isolate* isolate,
				v8::Local<v8::Context> context,
				Provider* provider,
				v8::Local<v8::Object> object) {
	v8::Local<v8::Array> keys;
	if (!object->GetOwnPropertyNames(context).ToLocal(&keys)) {
		return;
	}
	for (uint32_t index = 0; index < keys->Length(); ++index) {
		v8::Local<v8::Value> key;
		v8::Local<v8::Value> value;
		if (!keys->Get(context, index).ToLocal(&key) || !object->Get(context, key).ToLocal(&value)) {
			continue;
		}
		provider->set(Engine::Helper::FromV8Str(isolate, key), Engine::Helper::FromV8Str(isolate, value));
	}
}

v8::Local<v8::FunctionTemplate> MakeProviderStoreTemplate(v8::Isolate* isolate);

void ProviderStoreConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	if (!args.IsConstructCall()) {
		v8::Local<v8::Function> constructor = MakeProviderStoreTemplate(isolate)->GetFunction(context).ToLocalChecked();
		v8::Local<v8::Value> initial_value = args.Length() > 0
			? v8::Local<v8::Value>(args[0])
			: v8::Local<v8::Value>(v8::Undefined(isolate));
		v8::Local<v8::Value> argv[1] = {initial_value};
		v8::Local<v8::Object> instance = constructor->NewInstance(context, args.Length() > 0 ? 1 : 0, argv).ToLocalChecked();
		args.GetReturnValue().Set(instance);
		return;
	}

	auto* provider = new Provider();
	if (args.Length() > 0 && args[0]->IsObject()) {
		PopulateProviderFromObject(isolate, context, provider, args[0].As<v8::Object>());
	}
	Engine::Helper::WrapPointer(args.This(), provider);
	Engine::Helper::RegisterWeakCleanup(isolate, args.This(), provider);
	args.GetReturnValue().Set(args.This());
}

void ProviderStoreSetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Provider* provider = UnwrapProviderStore(args);
	if (provider == nullptr) {
		return;
	}
	std::string key;
	std::string value;
	if (!RequireStringArg(args, 0, "ProviderStore.set expects key string", &key)
		|| !RequireStringArg(args, 1, "ProviderStore.set expects value string", &value)) {
		return;
	}
	provider->set(std::move(key), std::move(value));
	args.GetReturnValue().Set(args.This());
}

void ProviderStoreGetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Provider* provider = UnwrapProviderStore(args);
	if (provider == nullptr) {
		return;
	}
	std::string key;
	if (!RequireStringArg(args, 0, "ProviderStore.get expects key string", &key)) {
		return;
	}
	const std::string* value = provider->get(key);
	if (value != nullptr) {
		args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), *value));
		return;
	}
	if (args.Length() > 1) {
		args.GetReturnValue().Set(args[1]);
		return;
	}
	args.GetReturnValue().Set(v8::Undefined(args.GetIsolate()));
}

void ProviderStoreKeysCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Provider* provider = UnwrapProviderStore(args);
	if (provider == nullptr) {
		return;
	}
	args.GetReturnValue().Set(MakeStringArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), provider->keys()));
}

v8::Local<v8::FunctionTemplate> MakeProviderStoreTemplate(v8::Isolate* isolate) {
	return Engine::Helper::MakeClass(
		isolate,
		"ProviderStore",
		&ProviderStoreConstructor,
		{{"set", &ProviderStoreSetCallback},
		 {"get", &ProviderStoreGetCallback},
		 {"keys", &ProviderStoreKeysCallback}});
}

void ProviderCreateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Function> constructor = MakeProviderStoreTemplate(isolate)->GetFunction(context).ToLocalChecked();
	v8::Local<v8::Value> initial_value = args.Length() > 0
		? v8::Local<v8::Value>(args[0])
		: v8::Local<v8::Value>(v8::Undefined(isolate));
	v8::Local<v8::Value> argv[1] = {initial_value};
	v8::Local<v8::Object> instance = constructor->NewInstance(context, args.Length() > 0 ? 1 : 0, argv).ToLocalChecked();
	args.GetReturnValue().Set(instance);
}

RuntimeLive::InterfaceCompilerOptions ParseRuntimeOptions(v8::Isolate* isolate,
					 v8::Local<v8::Context> context,
					 const v8::FunctionCallbackInfo<v8::Value>& args,
					 int index) {
	RuntimeLive::InterfaceCompilerOptions options;
	options.cache_dir = (std::filesystem::current_path() / "out/runtime_live_module_cache").string();
	options.provider = RuntimeLive::kProviderGPUToolkit;
	options.language = "cpp";
	if (args.Length() <= index || !args[index]->IsObject()) {
		return options;
	}
	v8::Local<v8::Object> object = args[index].As<v8::Object>();
	options.cache_dir = GetObjectString(isolate, context, object, "cacheDir", "cache_dir", options.cache_dir);
	options.provider = GetObjectString(isolate, context, object, "provider", nullptr, options.provider);
	options.language = GetObjectString(isolate, context, object, "language", nullptr, options.language);
	options.compile_flags = GetObjectString(isolate, context, object, "compileFlags", "compile_flags", "");
	options.link_flags = GetObjectString(isolate, context, object, "linkFlags", "link_flags", "");
	return options;
}

CompilerSource BuildCompilerSourceFromArgs(v8::Isolate* isolate,
					   v8::Local<v8::Context> context,
					   const v8::FunctionCallbackInfo<v8::Value>& args,
					   std::string* source_out) {
	std::string source = source_out == nullptr ? std::string() : *source_out;
	if (source_out != nullptr) {
		source = *source_out;
	}
	const RuntimeLive::InterfaceCompilerOptions options = ParseRuntimeOptions(isolate, context, args, 1);
	return CompilerSource(source,
			      options.cache_dir,
			      options.provider,
			      options.language,
			      options.compile_flags,
			      options.link_flags);
}

void RuntimeLiveDescribeCompilerCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string source;
	if (!RequireStringArg(args, 0, "describeCompiler expects source string", &source)) {
		return;
	}
	const RuntimeLive::InterfaceCompilerOptions options = ParseRuntimeOptions(isolate, context, args, 1);
	const CompilerSource compiler_source(source,
					 options.cache_dir,
					 options.provider,
					 options.language,
					 options.compile_flags,
					 options.link_flags);
	v8::Local<v8::Object> description = v8::Object::New(isolate);
	SetProperty(isolate, context, description, "cacheDir", Engine::Helper::ToV8Str(isolate, compiler_source.cache_dir().string()));
	SetProperty(isolate, context, description, "sourcePath", Engine::Helper::ToV8Str(isolate, compiler_source.source_path().string()));
	SetProperty(isolate, context, description, "binaryPath", Engine::Helper::ToV8Str(isolate, compiler_source.binary_path().string()));
	SetProperty(isolate, context, description, "compileOutPath", Engine::Helper::ToV8Str(isolate, compiler_source.compile_out_path().string()));
	SetProperty(isolate, context, description, "compileErrPath", Engine::Helper::ToV8Str(isolate, compiler_source.compile_err_path().string()));
	SetProperty(isolate, context, description, "runOutPath", Engine::Helper::ToV8Str(isolate, compiler_source.run_out_path().string()));
	SetProperty(isolate, context, description, "runErrPath", Engine::Helper::ToV8Str(isolate, compiler_source.run_err_path().string()));
	SetProperty(isolate, context, description, "useCpp", v8::Boolean::New(isolate, compiler_source.use_cpp()));
	SetProperty(isolate, context, description, "compilerArgs", MakeStringArray(isolate, context, compiler_source.BuildCompilerArgs()));
	args.GetReturnValue().Set(description);
}

void RuntimeLiveBuildCommandPreviewCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string source;
	if (!RequireStringArg(args, 0, "buildCompilerCommandPreview expects source string", &source)) {
		return;
	}
	const RuntimeLive::InterfaceCompilerOptions options = ParseRuntimeOptions(isolate, context, args, 1);
	const std::string compiler_binary = OptionalStringArg(args, 2, options.language == "c" ? "clang" : "clang++");
	const CompilerSource compiler_source(source,
					 options.cache_dir,
					 options.provider,
					 options.language,
					 options.compile_flags,
					 options.link_flags);
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(isolate, compiler_source.BuildCompilerCommandPreview(compiler_binary)));
}

void RuntimeLiveDefaultProviderCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), RuntimeLive::kProviderGPUToolkit));
}

bool BuildContainerModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  v8::Local<v8::Object>* module_out,
			  std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "sourceRoot", &ContainerSourceRootCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "listSourceFiles", &ContainerListSourceFilesCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "listSourceDirectories", &ContainerListSourceDirectoriesCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "readSourceFile", &ContainerReadSourceFileCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "findSourceFiles", &ContainerFindSourceFilesCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "describeSourceTree", &ContainerDescribeSourceTreeCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "availableModules", &ContainerAvailableModulesCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Container module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

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

bool BuildProviderModule(v8::Isolate* isolate,
			      v8::Local<v8::Context> context,
			      v8::Local<v8::Object>* module_out,
			      std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	v8::Local<v8::Function> provider_store = MakeProviderStoreTemplate(isolate)->GetFunction(context).ToLocalChecked();
	bool ok = true;
	ok = ok && SetProperty(isolate, context, module, "ProviderStore", provider_store);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "create", &ProviderCreateCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Provider module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

bool BuildRuntimeLiveModule(v8::Isolate* isolate,
			     v8::Local<v8::Context> context,
			     v8::Local<v8::Object>* module_out,
			     std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && SetProperty(isolate, context, module, "gpuToolkitProvider", Engine::Helper::ToV8Str(isolate, RuntimeLive::kProviderGPUToolkit));
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "defaultProvider", &RuntimeLiveDefaultProviderCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "describeCompiler", &RuntimeLiveDescribeCompilerCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "buildCompilerCommandPreview", &RuntimeLiveBuildCommandPreviewCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build RuntimeLive module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace

std::string ResolveCanonicalModuleName(const std::string& module_name) {
	const std::string normalized = NormalizeModuleKey(module_name);
	if (normalized == "container" || normalized == "project" || normalized == "sourcetree") {
		return "Container";
	}
	if (normalized == "filesystem" || normalized == "fs") {
		return "FileSystem";
	}
	if (normalized == "util" || normalized == "utils") {
		return "Util";
	}
	if (normalized == "provider") {
		return "Provider";
	}
	if (normalized == "runtimelive" || normalized == "live") {
		return "RuntimeLive";
	}
	return std::string();
}

std::vector<std::string> ListModules() {
	return {"Container", "FileSystem", "Provider", "RuntimeLive", "Util"};
}

bool ImportModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  const std::string& module_name,
			  std::string* error_out) {
	const std::string canonical_name = ResolveCanonicalModuleName(module_name);
	if (canonical_name.empty()) {
		if (error_out != nullptr) {
			*error_out = "unknown module: " + module_name;
		}
		return false;
	}

	v8::Local<v8::Object> module;
	bool ok = false;
	if (canonical_name == "Container") {
		ok = BuildContainerModule(isolate, context, &module, error_out);
	} else if (canonical_name == "FileSystem") {
		ok = BuildFileSystemModule(isolate, context, &module, error_out);
	} else if (canonical_name == "Util") {
		ok = BuildUtilModule(isolate, context, &module, error_out);
	} else if (canonical_name == "Provider") {
		ok = BuildProviderModule(isolate, context, &module, error_out);
	} else if (canonical_name == "RuntimeLive") {
		ok = BuildRuntimeLiveModule(isolate, context, &module, error_out);
	}
	if (!ok) {
		if (error_out != nullptr && error_out->empty()) {
			*error_out = "failed to initialize module: " + canonical_name;
		}
		return false;
	}

	if (!Engine::Helper::ExportGlobalModule(isolate, context, canonical_name.c_str(), module)) {
		if (error_out != nullptr) {
			*error_out = "failed to export module to global scope: " + canonical_name;
		}
		return false;
	}
	return true;
}

}  // namespace modules