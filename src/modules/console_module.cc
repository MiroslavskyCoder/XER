#include "modules/console_module.h"

#include <iostream>
#include <string>

namespace {

std::string JoinArguments(v8::Isolate* isolate, const v8::FunctionCallbackInfo<v8::Value>& args) {
    std::string text;
    for (int i = 0; i < args.Length(); ++i) {
        if (i > 0) {
            text += " ";
        }
        v8::String::Utf8Value arg(isolate, args[i]);
        if (*arg != nullptr) {
            text += *arg;
        }
    }
    return text;
}

void LogCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    std::cout << JoinArguments(args.GetIsolate(), args) << "\n";
}

void ErrorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    std::cerr << JoinArguments(args.GetIsolate(), args) << "\n";
}

void WarnCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    std::cerr << "[warn] " << JoinArguments(args.GetIsolate(), args) << "\n";
}

void InfoCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    std::cout << "[info] " << JoinArguments(args.GetIsolate(), args) << "\n";
}

}  // namespace

namespace modules {

bool RegisterConsoleModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> console_obj = v8::Object::New(isolate);
    bool ok = console_obj
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "log"),
                        v8::Function::New(context, LogCallback).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && console_obj
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "error"),
                         v8::Function::New(context, ErrorCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && console_obj
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "warn"),
                         v8::Function::New(context, WarnCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && console_obj
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "info"),
                         v8::Function::New(context, InfoCallback).ToLocalChecked())
                   .FromMaybe(false);

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Console"), console_obj)
        .FromMaybe(false);
}

}  // namespace modules
