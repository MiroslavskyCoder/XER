#include "flow_script_console.h"

#include <absl/strings/str_cat.h>
#include <absl/strings/str_format.h>

#include <chrono>
#include <string>
#include <unordered_map>

#include "flux/terminal/terminal_output_renderer.h"
#include "flux/v8_console_table.h"
#include "flux/v8_console_runtime.h"
#include "helper/class_builder.h"

namespace flow_script_detail {
namespace {

using ConsoleClock = std::chrono::steady_clock;

std::unordered_map<v8::Isolate*, std::unordered_map<std::string, ConsoleClock::time_point>>& ConsoleTimers() {
	static std::unordered_map<v8::Isolate*, std::unordered_map<std::string, ConsoleClock::time_point>> timers;
	return timers;
}

std::unordered_map<std::string, ConsoleClock::time_point>& TimersFor(v8::Isolate* isolate) {
	return ConsoleTimers()[isolate];
}

std::string DefaultConsoleLabel(v8::Isolate* isolate,
					const v8::FunctionCallbackInfo<v8::Value>& args,
					int index) {
	if (index >= args.Length() || args[index]->IsUndefined()) {
		return "default";
	}
	return Utf8(isolate, args[index]);
}


flux::terminal::OutputStream ParseOutputStream(v8::Isolate* isolate,
					       const v8::FunctionCallbackInfo<v8::Value>& args,
					       int index) {
	if (index >= args.Length() || args[index]->IsUndefined()) {
		return flux::terminal::OutputStream::kStdout;
	}
	return Utf8(isolate, args[index]) == "stderr"
		? flux::terminal::OutputStream::kStderr
		: flux::terminal::OutputStream::kStdout;
}

std::string BuildConsoleTrace(v8::Isolate* isolate) {
	v8::Local<v8::StackTrace> stack = v8::StackTrace::CurrentStackTrace(isolate, 32, v8::StackTrace::kDetailed);
	if (stack.IsEmpty() || stack->GetFrameCount() == 0) {
		return std::string();
	}
	std::string out;
	for (int index = 0; index < stack->GetFrameCount(); ++index) {
		v8::Local<v8::StackFrame> frame = stack->GetFrame(isolate, index);
		if (frame.IsEmpty()) {
			continue;
		}
		v8::Local<v8::String> function_name_value = frame->GetFunctionName();
		v8::Local<v8::String> script_name_value = frame->GetScriptNameOrSourceURL();
		const std::string function_name = function_name_value.IsEmpty() ? std::string() : Utf8(isolate, function_name_value);
		std::string script_name = script_name_value.IsEmpty() ? std::string() : Utf8(isolate, script_name_value);
		if (script_name.empty()) {
			script_name = "<anonymous>";
		}
		if (!out.empty()) {
			absl::StrAppend(&out, "\n");
		}
		if (function_name.empty()) {
			absl::StrAppendFormat(&out, "    at %s:%d:%d", script_name, frame->GetLineNumber(), frame->GetColumn());
		} else {
			absl::StrAppendFormat(
				&out,
				"    at %s (%s:%d:%d)",
				function_name,
				script_name,
				frame->GetLineNumber(),
				frame->GetColumn());
		}
	}
	return out;
}

void ConsoleConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	if (!args.IsConstructCall()) {
		v8::Local<v8::Value> console_ctor_value;
		if (!context->Global()->Get(context, Engine::Helper::ToV8Str(isolate, "Console")).ToLocal(&console_ctor_value)
			|| !console_ctor_value->IsFunction()) {
			Engine::Helper::ThrowError(isolate, "Console constructor is unavailable");
			return;
		}
		v8::Local<v8::Object> instance = console_ctor_value.As<v8::Function>()->NewInstance(context).ToLocalChecked();
		args.GetReturnValue().Set(instance);
		return;
	}
	args.GetReturnValue().Set(args.This());
}

std::string JoinArgumentsFromIndex(v8::Isolate* isolate,
				    const v8::FunctionCallbackInfo<v8::Value>& args,
				    int start_index) {
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::vector<v8::Local<v8::Value>> values;
	for (int index = start_index; index < args.Length(); ++index) {
		values.push_back(args[index]);
	}
	return flux::console::JoinValues(isolate, context, values);
}

v8::Local<v8::FunctionTemplate> MakeConsoleTemplate(v8::Isolate* isolate) {
	return Engine::Helper::MakeClass(
		isolate,
		"Console",
		&ConsoleConstructor,
		{{"log", &ConsoleLogCallback},
		 {"info", &ConsoleInfoCallback},
		 {"warn", &ConsoleWarnCallback},
		 {"error", &ConsoleErrorCallback},
		 {"dir", &ConsoleDirCallback},
		 {"assert", &ConsoleAssertCallback},
		 {"table", &ConsoleTableCallback},
		 {"time", &ConsoleTimeCallback},
		 {"timeEnd", &ConsoleTimeEndCallback},
		 {"trace", &ConsoleTraceCallback},
		 {"snapshot", &ConsoleSnapshotCallback},
		 {"clearSnapshot", &ConsoleClearSnapshotCallback}},
		{},
		{},
		0);
}

}  // namespace

bool BindConsoleGlobals(v8::Isolate* isolate, v8::Local<v8::Context> context) {
	v8::Local<v8::FunctionTemplate> console_template = MakeConsoleTemplate(isolate);
	if (!Engine::Helper::ExportClass(isolate, context, "Console", console_template)) {
		return false;
	}
	v8::Local<v8::Value> console_ctor_value;
	if (!context->Global()->Get(context, Engine::Helper::ToV8Str(isolate, "Console")).ToLocal(&console_ctor_value)
		|| !console_ctor_value->IsFunction()) {
		return false;
	}
	v8::Local<v8::Object> console_instance = console_ctor_value.As<v8::Function>()->NewInstance(context).ToLocalChecked();
	return context->Global()
		->Set(context, Engine::Helper::ToV8Str(isolate, "console"), console_instance)
		.FromMaybe(false);
}

std::string Utf8(v8::Isolate* isolate, v8::Local<v8::Value> value) {
	return flux::console::Utf8(isolate, value);
}

std::string JoinArguments(v8::Isolate* isolate, const v8::FunctionCallbackInfo<v8::Value>& args) {
	return flux::console::JoinArguments(isolate, args);
}

void ConsoleLogCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	flux::console::WriteLine(flux::console::Stream::kStdout, JoinArguments(args.GetIsolate(), args));
}

void ConsoleInfoCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	flux::console::WriteLine(flux::console::Stream::kStdout, JoinArguments(args.GetIsolate(), args));
}

void ConsoleWarnCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	flux::console::WriteLine(flux::console::Stream::kStderr, JoinArguments(args.GetIsolate(), args));
}

void ConsoleErrorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	flux::console::WriteLine(flux::console::Stream::kStderr, JoinArguments(args.GetIsolate(), args));
}

void ConsoleDirCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	flux::console::WriteLine(flux::console::Stream::kStdout, JoinArguments(args.GetIsolate(), args));
}

void ConsoleAssertCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	if (args.Length() > 0 && args[0]->BooleanValue(isolate)) {
		return;
	}
	const std::string suffix = args.Length() > 1 ? JoinArgumentsFromIndex(isolate, args, 1) : std::string();
	const std::string message = suffix.empty() ? "Assertion failed" : "Assertion failed: " + suffix;
	flux::console::WriteLine(flux::console::Stream::kStderr, message);
}

void ConsoleTableCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	if (args.Length() == 0) {
		return;
	}
	flux::console::WriteLine(
		flux::console::Stream::kStdout,
		flux::console::RenderTable(
			isolate,
			context,
			args[0],
			args.Length() > 1 ? args[1] : v8::Undefined(isolate)));
}

void ConsoleTimeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	TimersFor(isolate)[DefaultConsoleLabel(isolate, args, 0)] = ConsoleClock::now();
}

void ConsoleTimeEndCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	const std::string label = DefaultConsoleLabel(isolate, args, 0);
	auto& timers = TimersFor(isolate);
	const auto timer_it = timers.find(label);
	if (timer_it == timers.end()) {
		flux::console::WriteLine(
			flux::console::Stream::kStderr,
			absl::StrCat("Timer '", label, "' does not exist"));
		return;
	}
	const double elapsed_ms = std::chrono::duration<double, std::milli>(ConsoleClock::now() - timer_it->second).count();
	timers.erase(timer_it);
	flux::console::WriteLine(
		flux::console::Stream::kStdout,
		absl::StrCat(label, ": ", absl::StrFormat("%.3fms", elapsed_ms)));
}

void ConsoleTraceCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	const std::string prefix = args.Length() > 0
		? absl::StrCat("Trace: ", JoinArguments(isolate, args))
		: std::string("Trace");
	const std::string stack = BuildConsoleTrace(isolate);
	flux::console::WriteLine(
		flux::console::Stream::kStderr,
		stack.empty() ? prefix : absl::StrCat(prefix, "\n", stack));
}

void ConsoleSnapshotCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	args.GetReturnValue().Set(
		Engine::Helper::ToV8Str(isolate, flux::terminal::Snapshot(ParseOutputStream(isolate, args, 0))));
}

void ConsoleClearSnapshotCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	if (args.Length() == 0 || args[0]->IsUndefined()) {
		flux::terminal::ClearSnapshot();
		return;
	}
	flux::terminal::ClearSnapshot(ParseOutputStream(isolate, args, 0));
}

}  // namespace flow_script_detail
