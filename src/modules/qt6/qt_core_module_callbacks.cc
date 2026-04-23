#include "modules/qt6/qt_core_module.h"

#include "wrapper/qt6/core/path.h"
#include "wrapper/qt6/util/v8_string.h"

namespace modules::qt_core_module_detail {

void CoreIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(
        v8::Boolean::New(args.GetIsolate(), qt6::core::IsAvailable()));
}

void CoreVersionCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(args.GetIsolate(),
            qt6::core::VersionString().c_str()).ToLocalChecked());
}

void CoreCleanPathCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        iso->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(iso, "cleanPath(path: string)")));
        return;
    }
    auto result = qt6::core::CleanPath(qt6::util::V8ValueToStdString(iso, args[0]));
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, result.c_str()).ToLocalChecked());
}

void CoreNativeSepCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        iso->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(iso, "toNativeSeparators(path: string)")));
        return;
    }
    auto result = qt6::core::ToNativeSeparators(qt6::util::V8ValueToStdString(iso, args[0]));
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, result.c_str()).ToLocalChecked());
}

void CoreAbsolutePathCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        iso->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(iso, "absolutePath(path: string)")));
        return;
    }
    auto result = qt6::core::AbsolutePath(qt6::util::V8ValueToStdString(iso, args[0]));
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, result.c_str()).ToLocalChecked());
}

void CoreDirNameCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    if (args.Length() < 1) return;
    auto result = qt6::core::DirName(qt6::util::V8ValueToStdString(iso, args[0]));
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, result.c_str()).ToLocalChecked());
}

void CoreBaseNameCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    if (args.Length() < 1) return;
    auto result = qt6::core::BaseName(qt6::util::V8ValueToStdString(iso, args[0]));
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, result.c_str()).ToLocalChecked());
}

void CoreExtensionCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    if (args.Length() < 1) return;
    auto result = qt6::core::Extension(qt6::util::V8ValueToStdString(iso, args[0]));
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, result.c_str()).ToLocalChecked());
}

void CoreJoinPathCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    if (args.Length() < 2) {
        iso->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(iso, "joinPath(base: string, child: string)")));
        return;
    }
    auto result = qt6::core::JoinPath(
        qt6::util::V8ValueToStdString(iso, args[0]),
        qt6::util::V8ValueToStdString(iso, args[1]));
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, result.c_str()).ToLocalChecked());
}

void CorePathExistsCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    if (args.Length() < 1) {
        args.GetReturnValue().Set(v8::Boolean::New(iso, false));
        return;
    }
    bool exists = qt6::core::PathExists(qt6::util::V8ValueToStdString(iso, args[0]));
    args.GetReturnValue().Set(v8::Boolean::New(iso, exists));
}

void CoreIsDirCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    if (args.Length() < 1) {
        args.GetReturnValue().Set(v8::Boolean::New(iso, false));
        return;
    }
    bool is_dir = qt6::core::IsDir(qt6::util::V8ValueToStdString(iso, args[0]));
    args.GetReturnValue().Set(v8::Boolean::New(iso, is_dir));
}

}  // namespace modules::qt_core_module_detail
