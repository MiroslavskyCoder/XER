#include "modules/module_builders.h"

#include "compilerapi/compiler_source.h"
#include "compilerapi/runtime_live.h"

#include <filesystem>
#include <string>
#include <vector>

namespace modules::detail {
namespace {

std::vector<RuntimeLive::InterfaceCompiler::SourceFile> GetRuntimeSourceFiles(
						v8::Isolate* isolate,
						v8::Local<v8::Context> context,
						v8::Local<v8::Object> object) {
	std::vector<RuntimeLive::InterfaceCompiler::SourceFile> files;
	v8::Local<v8::Value> value;
	if (!GetObjectValue(isolate, context, object, "sourceFiles", "source_files", &value) || !value->IsArray()) {
		return files;
	}
	v8::Local<v8::Array> array = value.As<v8::Array>();
	for (uint32_t index = 0; index < array->Length(); ++index) {
		v8::Local<v8::Value> element;
		if (!array->Get(context, index).ToLocal(&element) || !element->IsObject()) {
			continue;
		}
		v8::Local<v8::Object> file_object = element.As<v8::Object>();
		RuntimeLive::InterfaceCompiler::SourceFile file;
		file.path = GetObjectString(isolate, context, file_object, "path", nullptr, std::string());
		file.content = GetObjectString(isolate, context, file_object, "content", nullptr, std::string());
		file.is_header = GetObjectBool(isolate, context, file_object, "isHeader", "is_header", false);
		if (!file.path.empty()) {
			files.push_back(std::move(file));
		}
	}
	return files;
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

v8::Local<v8::Object> MakeRuntimeExecutionObject(v8::Isolate* isolate,
						 v8::Local<v8::Context> context,
						 const RuntimeLive::ExecutionResult& result) {
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	v8::Local<v8::Object> summary = v8::Object::New(isolate);
	v8::Local<v8::Object> artifacts = v8::Object::New(isolate);
	v8::Local<v8::Object> outputs = v8::Object::New(isolate);

	SetProperty(isolate, context, object, "ok", v8::Boolean::New(isolate, result.ok));
	SetProperty(isolate, context, object, "error", Engine::Helper::ToV8Str(isolate, result.error));

	SetProperty(isolate, context, summary, "compileInvoked", v8::Boolean::New(isolate, result.summary.compile_invoked));
	SetProperty(isolate, context, summary, "runInvoked", v8::Boolean::New(isolate, result.summary.run_invoked));
	SetProperty(isolate, context, summary, "compileExitCode", v8::Integer::New(isolate, result.summary.compile_exit_code));
	SetProperty(isolate, context, summary, "runExitCode", v8::Integer::New(isolate, result.summary.run_exit_code));
	SetProperty(isolate, context, summary, "sourceUnits", v8::Integer::New(isolate, result.summary.source_units));
	SetProperty(isolate, context, summary, "compilerBinary", Engine::Helper::ToV8Str(isolate, result.summary.compiler_binary));
	SetProperty(isolate, context, summary, "compilerCommand", Engine::Helper::ToV8Str(isolate, result.summary.compiler_command));
	SetProperty(isolate, context, summary, "compilerArgs", MakeStringArray(isolate, context, result.compiler_args));

	SetProperty(isolate, context, artifacts, "cacheDir", Engine::Helper::ToV8Str(isolate, result.cache_dir));
	SetProperty(isolate, context, artifacts, "sourcePath", Engine::Helper::ToV8Str(isolate, result.source_path));
	SetProperty(isolate, context, artifacts, "binaryPath", Engine::Helper::ToV8Str(isolate, result.binary_path));
	SetProperty(isolate, context, artifacts, "compileOutPath", Engine::Helper::ToV8Str(isolate, result.compile_out_path));
	SetProperty(isolate, context, artifacts, "compileErrPath", Engine::Helper::ToV8Str(isolate, result.compile_err_path));
	SetProperty(isolate, context, artifacts, "runOutPath", Engine::Helper::ToV8Str(isolate, result.run_out_path));
	SetProperty(isolate, context, artifacts, "runErrPath", Engine::Helper::ToV8Str(isolate, result.run_err_path));

	SetProperty(isolate, context, outputs, "compileStdout", Engine::Helper::ToV8Str(isolate, result.compile_stdout));
	SetProperty(isolate, context, outputs, "compileStderr", Engine::Helper::ToV8Str(isolate, result.compile_stderr));
	SetProperty(isolate, context, outputs, "runStdout", Engine::Helper::ToV8Str(isolate, result.run_stdout));
	SetProperty(isolate, context, outputs, "runStderr", Engine::Helper::ToV8Str(isolate, result.run_stderr));

	SetProperty(isolate, context, object, "summary", summary);
	SetProperty(isolate, context, object, "artifacts", artifacts);
	SetProperty(isolate, context, object, "outputs", outputs);
	return object;
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

void RuntimeLiveCompileAndRunCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	const RuntimeLive::InterfaceCompilerOptions options = ParseRuntimeOptions(isolate, context, args, 1);
	const std::vector<RuntimeLive::InterfaceCompiler::SourceFile> source_files =
		(args.Length() > 1 && args[1]->IsObject())
			? GetRuntimeSourceFiles(isolate, context, args[1].As<v8::Object>())
			: std::vector<RuntimeLive::InterfaceCompiler::SourceFile>();
	const std::string source = (args.Length() > 0 && args[0]->IsString())
		? Engine::Helper::FromV8Str(isolate, args[0])
		: std::string();
	if (source.empty() && source_files.empty()) {
		Engine::Helper::ThrowTypeError(isolate, "compileAndRun expects source string or options.sourceFiles");
		return;
	}

	RuntimeLive::InterfaceCompiler compiler = RuntimeLive::CreateInterfaceCompiler(options);
	if (!source_files.empty()) {
		for (const auto& source_file : source_files) {
			compiler.AddEntryFile(source_file);
		}
	} else {
		compiler.AddEntryRaw(source);
	}

	RuntimeLive::ExecutionResult result;
	compiler.CompileAndRun(&result, nullptr);
	args.GetReturnValue().Set(MakeRuntimeExecutionObject(isolate, context, result));
}

}  // namespace

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
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "compileAndRun", &RuntimeLiveCompileAndRunCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build RuntimeLive module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail