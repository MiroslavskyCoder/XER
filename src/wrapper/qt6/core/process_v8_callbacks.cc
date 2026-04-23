#include "wrapper/qt6/core/process.h"

#include "wrapper/qt6/v8/async_bridge.h"
#include "wrapper/qt6/v8/class_builder.h"
#include "wrapper/qt6/util/v8_string.h"

namespace qt6::core::process_v8_detail {

using namespace qt6::v8bridge;

ProcessWrapper* GetSelf(const v8::FunctionCallbackInfo<v8::Value>& a) {
    return UnwrapPointer<ProcessWrapper>(a.This());
}

void ProcCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtProcess()'");
        return;
    }
    auto* w = new ProcessWrapper();
    WrapPointer(args.This(), w);
    RegisterWeakCleanup(args.GetIsolate(), args.This(), w);
    args.GetReturnValue().Set(args.This());
}

void ProcSetWorkDir(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    s->setWorkDir(FromV8Str(a.GetIsolate(), a[0]));
}

void ProcSetEnv(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 2) return;
    s->setEnv(FromV8Str(a.GetIsolate(), a[0]), FromV8Str(a.GetIsolate(), a[1]));
}

void ProcClearEnv(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) s->clearEnv();
}

void ProcRun(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    auto iso = a.GetIsolate();
    auto ctx = iso->GetCurrentContext();

    if (a.Length() < 1 || !a[0]->IsString()) {
        ThrowTypeError(iso, "run(program: string, args?: string[], timeout?: number)");
        return;
    }
    std::string program = FromV8Str(iso, a[0]);
    std::vector<std::string> args;
    int timeout = 30000;

    if (a.Length() >= 2 && a[1]->IsArray()) {
        auto arr = a[1].As<v8::Array>();
        for (uint32_t i = 0; i < arr->Length(); ++i) {
            auto elem = arr->Get(ctx, i);
            if (!elem.IsEmpty()) args.push_back(FromV8Str(iso, elem.ToLocalChecked()));
        }
    }
    if (a.Length() >= 3 && a[2]->IsNumber()) timeout = a[2]->Int32Value(ctx).FromMaybe(30000);

    auto result = s->run(program, args, timeout);

    auto obj = v8::Object::New(iso);
    obj->Set(ctx, ToV8Str(iso, "ok"), v8::Boolean::New(iso, result.ok)).Check();
    obj->Set(ctx, ToV8Str(iso, "exitCode"), v8::Integer::New(iso, result.exit_code)).Check();
    obj->Set(ctx, ToV8Str(iso, "stdout"), ToV8Str(iso, result.stdout_out)).Check();
    obj->Set(ctx, ToV8Str(iso, "stderr"), ToV8Str(iso, result.stderr_out)).Check();
    if (!result.error.empty()) obj->Set(ctx, ToV8Str(iso, "error"), ToV8Str(iso, result.error)).Check();
    a.GetReturnValue().Set(obj);
}

void ProcRunAsync(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    auto iso = a.GetIsolate();
    auto ctx = iso->GetCurrentContext();

    if (a.Length() < 3 || !a[0]->IsString() || !a[2]->IsFunction()) {
        ThrowTypeError(iso, "runAsync(program, args[], callback, timeout?)");
        return;
    }

    std::string program = FromV8Str(iso, a[0]);
    std::vector<std::string> args;
    if (a[1]->IsArray()) {
        auto arr = a[1].As<v8::Array>();
        for (uint32_t i = 0; i < arr->Length(); ++i) {
            auto elem = arr->Get(ctx, i);
            if (!elem.IsEmpty()) args.push_back(FromV8Str(iso, elem.ToLocalChecked()));
        }
    }
    auto cb = a[2].As<v8::Function>();
    int timeout = a.Length() >= 4 ? a[3]->Int32Value(ctx).FromMaybe(30000) : 30000;

    const ProcessWrapper* runner = s;

    qt6::async::PostAsync<ProcessResult>(
        iso,
        cb,
        ctx,
        [runner, program, args, timeout](std::string& out_error) -> ProcessResult {
            auto r = runner->run(program, args, timeout);
            if (!r.ok && !r.error.empty()) out_error = r.error;
            return r;
        },
        [](v8::Isolate* iso, const ProcessResult& r) -> v8::Local<v8::Value> {
            auto ctx = iso->GetCurrentContext();
            auto obj = v8::Object::New(iso);
            obj->Set(ctx, ToV8Str(iso, "ok"), v8::Boolean::New(iso, r.ok)).Check();
            obj->Set(ctx, ToV8Str(iso, "exitCode"), v8::Integer::New(iso, r.exit_code)).Check();
            obj->Set(ctx, ToV8Str(iso, "stdout"), ToV8Str(iso, r.stdout_out)).Check();
            obj->Set(ctx, ToV8Str(iso, "stderr"), ToV8Str(iso, r.stderr_out)).Check();
            return obj;
        });
}

}  // namespace qt6::core::process_v8_detail
