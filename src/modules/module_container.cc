#include "modules/module_builders.h"

#include "helper/tool_to.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace modules::detail {
namespace {

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

}  // namespace

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

}  // namespace modules::detail