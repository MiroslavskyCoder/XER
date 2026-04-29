#include "modules/module_builders.h"

#include "analysis/analysis_engine.h"
#include "core/engine_doctor_config.h"
#include "core/engine_doctor_context.h"
#include "scanner/scan_parameters.h"
#include "scanner/scan_result.h"
#include "scanner/scanner_module.h"

#include <string>
#include <vector>

namespace modules::detail {
namespace {

std::string ScanStatusToString(EngineDoctor::ScanStatus status) {
	switch (status) {
	case EngineDoctor::ScanStatus::PENDING:
		return "pending";
	case EngineDoctor::ScanStatus::SUCCESS:
		return "success";
	case EngineDoctor::ScanStatus::WARNING:
		return "warning";
	case EngineDoctor::ScanStatus::ERROR:
		return "error";
	case EngineDoctor::ScanStatus::SKIPPED:
		return "skipped";
	}
	return "unknown";
}

v8::Local<v8::Object> MakeScanMessageObject(v8::Isolate* isolate,
					    v8::Local<v8::Context> context,
					    const EngineDoctor::ScanMessage& message) {
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "code", Engine::Helper::ToV8Str(isolate, message.code));
	SetProperty(isolate, context, object, "message", Engine::Helper::ToV8Str(isolate, message.message));
	return object;
}

v8::Local<v8::Array> MakeScanMessagesArray(v8::Isolate* isolate,
					   v8::Local<v8::Context> context,
					   const std::vector<EngineDoctor::ScanMessage>& messages) {
	v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(messages.size()));
	for (size_t index = 0; index < messages.size(); ++index) {
		array->Set(context, static_cast<uint32_t>(index), MakeScanMessageObject(isolate, context, messages[index])).FromMaybe(false);
	}
	return array;
}

v8::Local<v8::Object> MakeScanResultObject(v8::Isolate* isolate,
					   v8::Local<v8::Context> context,
					   const EngineDoctor::ScanResult& result) {
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	v8::Local<v8::Object> metadata = v8::Object::New(isolate);
	SetProperty(isolate, context, object, "filePath", Engine::Helper::ToV8Str(isolate, result.file_path));
	SetProperty(isolate, context, object, "status", Engine::Helper::ToV8Str(isolate, ScanStatusToString(result.status)));
	SetProperty(isolate, context, object, "errorMessage", Engine::Helper::ToV8Str(isolate, result.error_message));
	SetProperty(isolate, context, object, "dependencies", MakeStringArray(isolate, context, result.dependencies));
	SetProperty(isolate, context, object, "errors", MakeScanMessagesArray(isolate, context, result.errors));
	SetProperty(isolate, context, object, "warnings", MakeScanMessagesArray(isolate, context, result.warnings));
	SetProperty(isolate, context, metadata, "fullPath", Engine::Helper::ToV8Str(isolate, result.metadata.full_path));
	SetProperty(isolate, context, metadata, "size", v8::Number::New(isolate, static_cast<double>(result.metadata.size)));
	SetProperty(isolate, context, metadata, "exists", v8::Boolean::New(isolate, result.metadata.exists));
	SetProperty(isolate, context, metadata, "isDirectory", v8::Boolean::New(isolate, result.metadata.is_directory));
	SetProperty(isolate, context, metadata, "extension", Engine::Helper::ToV8Str(isolate, result.metadata.extension));
	SetProperty(isolate, context, object, "metadata", metadata);
	return object;
}

v8::Local<v8::Object> MakeAnalysisSummaryObject(v8::Isolate* isolate,
						v8::Local<v8::Context> context,
						const EngineDoctor::AnalysisSummary& summary) {
	v8::Local<v8::Object> object = v8::Object::New(isolate);
	v8::Local<v8::Object> metrics = v8::Object::New(isolate);
	v8::Local<v8::Array> task_summaries = v8::Array::New(isolate, static_cast<int>(summary.task_summaries.size()));
	SetProperty(isolate, context, metrics, "totalFiles", v8::Number::New(isolate, static_cast<double>(summary.metrics.total_files)));
	SetProperty(isolate, context, metrics, "successfulFiles", v8::Number::New(isolate, static_cast<double>(summary.metrics.successful_files)));
	SetProperty(isolate, context, metrics, "filesWithWarnings", v8::Number::New(isolate, static_cast<double>(summary.metrics.files_with_warnings)));
	SetProperty(isolate, context, metrics, "failedFiles", v8::Number::New(isolate, static_cast<double>(summary.metrics.failed_files)));
	SetProperty(isolate, context, metrics, "totalErrors", v8::Number::New(isolate, static_cast<double>(summary.metrics.total_errors)));
	SetProperty(isolate, context, metrics, "totalWarnings", v8::Number::New(isolate, static_cast<double>(summary.metrics.total_warnings)));
	SetProperty(isolate, context, metrics, "totalDependencies", v8::Number::New(isolate, static_cast<double>(summary.metrics.total_dependencies)));
	SetProperty(isolate, context, metrics, "emptyFiles", v8::Number::New(isolate, static_cast<double>(summary.metrics.empty_files)));
	for (size_t index = 0; index < summary.task_summaries.size(); ++index) {
		const auto& task = summary.task_summaries[index];
		v8::Local<v8::Object> task_object = v8::Object::New(isolate);
		SetProperty(isolate, context, task_object, "taskName", Engine::Helper::ToV8Str(isolate, task.task_name));
		SetProperty(isolate, context, task_object, "affectedFiles", v8::Number::New(isolate, static_cast<double>(task.affected_files)));
		SetProperty(isolate, context, task_object, "issueCount", v8::Number::New(isolate, static_cast<double>(task.issue_count)));
		SetProperty(isolate, context, task_object, "description", Engine::Helper::ToV8Str(isolate, task.description));
		task_summaries->Set(context, static_cast<uint32_t>(index), task_object).FromMaybe(false);
	}
	SetProperty(isolate, context, object, "generatedAtUtc", Engine::Helper::ToV8Str(isolate, summary.generated_at_utc));
	SetProperty(isolate, context, object, "metrics", metrics);
	SetProperty(isolate, context, object, "taskSummaries", task_summaries);
	SetProperty(isolate, context, object, "dominantProblemCodes", MakeStringArray(isolate, context, summary.dominant_problem_codes));
	return object;
}

void DoctorRunCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	EngineDoctor::Config config;
	EngineDoctor::ScanParameters params;
	params.paths_to_scan = {ResolveProjectRoot().string()};
	if (args.Length() > 0 && args[0]->IsObject()) {
		v8::Local<v8::Object> options = args[0].As<v8::Object>();
		const std::vector<std::string> requested_paths = GetObjectStringArray(isolate, context, options, "paths", nullptr);
		if (!requested_paths.empty()) {
			params.paths_to_scan.clear();
			for (const auto& path : requested_paths) {
				params.paths_to_scan.push_back(ResolveProjectRelativePath(path).string());
			}
		}
		params.analyze_syntax = GetObjectBool(isolate, context, options, "analyzeSyntax", "analyze_syntax", true);
		params.analyze_dependencies = GetObjectBool(isolate, context, options, "analyzeDependencies", "analyze_dependencies", true);
		params.analyze_security = GetObjectBool(isolate, context, options, "analyzeSecurity", "analyze_security", false);
		params.include_hidden = GetObjectBool(isolate, context, options, "includeHidden", "include_hidden", false);
		config.log_level = GetObjectBool(isolate, context, options, "verbose", nullptr, false) ? 0 : 2;
	} else {
		config.log_level = 2;
	}

	EngineDoctor::Context doctor_context;
	EngineDoctor::ScannerModule scanner(doctor_context);
	EngineDoctor::AnalysisEngine analyzer(doctor_context);
	scanner.initialize(config);
	analyzer.initialize(config);
	scanner.scan(params);
	analyzer.analyze();

	const auto* scan_results = doctor_context.get_data<std::vector<EngineDoctor::ScanResult>>("scan_results");
	const auto* summary = doctor_context.get_data<EngineDoctor::AnalysisSummary>("analysis_summary");
	const auto* summary_text = doctor_context.get_data<std::string>("analysis_summary_text");
	if (scan_results == nullptr || summary == nullptr || summary_text == nullptr) {
		Engine::Helper::ThrowError(isolate, "doctor analysis did not produce a complete result");
		return;
	}

	v8::Local<v8::Object> object = v8::Object::New(isolate);
	v8::Local<v8::Array> scan_array = v8::Array::New(isolate, static_cast<int>(scan_results->size()));
	for (size_t index = 0; index < scan_results->size(); ++index) {
		scan_array->Set(context, static_cast<uint32_t>(index), MakeScanResultObject(isolate, context, (*scan_results)[index])).FromMaybe(false);
	}
	SetProperty(isolate, context, object, "projectRoot", Engine::Helper::ToV8Str(isolate, ResolveProjectRoot().string()));
	SetProperty(isolate, context, object, "sourceRoot", Engine::Helper::ToV8Str(isolate, ResolveSourceRoot().string()));
	SetProperty(isolate, context, object, "summary", MakeAnalysisSummaryObject(isolate, context, *summary));
	SetProperty(isolate, context, object, "summaryText", Engine::Helper::ToV8Str(isolate, *summary_text));
	SetProperty(isolate, context, object, "scanResults", scan_array);
	args.GetReturnValue().Set(object);
}

}  // namespace

bool BuildDoctorModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "run", &DoctorRunCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Doctor module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail