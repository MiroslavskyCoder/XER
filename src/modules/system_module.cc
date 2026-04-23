#include "modules/system_module.h"

#include <cstdlib>
#include <filesystem>
#include <string>

namespace {

std::string ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return *utf8;
}

void CwdCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    std::error_code ec;
    const std::filesystem::path cwd = std::filesystem::current_path(ec);
    if (ec) {
        args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(args.GetIsolate(), ""));
        return;
    }

    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(args.GetIsolate(), cwd.string().c_str()).ToLocalChecked());
}

void ChdirCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 1 || !args[0]->IsString()) {
        args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
        return;
    }

    std::error_code ec;
    std::filesystem::current_path(ValueToString(args.GetIsolate(), args[0]), ec);
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), !ec));
}

void ExistsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 1 || !args[0]->IsString()) {
        args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
        return;
    }

    std::error_code ec;
    const bool exists = std::filesystem::exists(ValueToString(args.GetIsolate(), args[0]), ec);
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), !ec && exists));
}

void PlatformCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if defined(_WIN32)
    constexpr const char* kPlatform = "windows";
#elif defined(__APPLE__)
    constexpr const char* kPlatform = "darwin";
#elif defined(__linux__)
    constexpr const char* kPlatform = "linux";
#else
    constexpr const char* kPlatform = "unknown";
#endif
    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), kPlatform).ToLocalChecked());
}

void GetEnvCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        args.GetReturnValue().Set(v8::Undefined(isolate));
        return;
    }

    const std::string key = ValueToString(isolate, args[0]);
    const char* value = std::getenv(key.c_str());
    if (value == nullptr) {
        args.GetReturnValue().Set(v8::Undefined(isolate));
        return;
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, value).ToLocalChecked());
}

void SetEnvCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
        args.GetReturnValue().Set(v8::Boolean::New(isolate, false));
        return;
    }

    const std::string key = ValueToString(isolate, args[0]);
    const std::string value = ValueToString(isolate, args[1]);
    int overwrite = 1;
    if (args.Length() > 2 && args[2]->IsBoolean()) {
        overwrite = args[2].As<v8::Boolean>()->Value() ? 1 : 0;
    }

    const int result = setenv(key.c_str(), value.c_str(), overwrite);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, result == 0));
}

void ExecCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        args.GetReturnValue().Set(v8::Integer::New(isolate, -1));
        return;
    }

    const std::string command = ValueToString(isolate, args[0]);
    const int code = std::system(command.c_str());
    args.GetReturnValue().Set(v8::Integer::New(isolate, code));
}

}  // namespace

namespace modules {

bool RegisterSystemModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> system = v8::Object::New(isolate);
    bool ok = system
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "cwd"),
                        v8::Function::New(context, CwdCallback).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && system
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "chdir"),
                         v8::Function::New(context, ChdirCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && system
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "exists"),
                         v8::Function::New(context, ExistsCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && system
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "platform"),
                         v8::Function::New(context, PlatformCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && system
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "getenv"),
                         v8::Function::New(context, GetEnvCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && system
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "setenv"),
                         v8::Function::New(context, SetEnvCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && system
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "exec"),
                         v8::Function::New(context, ExecCallback).ToLocalChecked())
                   .FromMaybe(false);

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "System"), system)
        .FromMaybe(false);
}

}  // namespace modules
