#include "flow_script_console.h"

#include "flux/v8_console_runtime.h"
#include "helper/class_builder.h"

namespace flow_script_detail {
namespace {

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
		 {"assert", &ConsoleAssertCallback}},
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

}  // namespace flow_script_detail
