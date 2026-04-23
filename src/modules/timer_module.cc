#include "modules/timer_module.h"

#include <chrono>
#include <thread>

namespace {

void NowMsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    using namespace std::chrono;
    const auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), static_cast<double>(now)));
}

void MonotonicMsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    using namespace std::chrono;
    const auto now = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), static_cast<double>(now)));
}

void SleepCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 1 || !args[0]->IsNumber()) {
        return;
    }

    int ms = args[0].As<v8::Number>()->Value();
    if (ms < 0) {
        ms = 0;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void SetTimeoutCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 2 || !args[0]->IsFunction() || !args[1]->IsNumber()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "setTimeout expects function and delay")));
        return;
    }

    int ms = args[1].As<v8::Number>()->Value();
    if (ms < 0) {
        ms = 0;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    (void)args[0].As<v8::Function>()->Call(context, context->Global(), 0, nullptr);
}

}  // namespace

namespace modules {

bool RegisterTimerModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> timer = v8::Object::New(isolate);
    bool ok = timer
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "nowMs"),
                        v8::Function::New(context, NowMsCallback).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && timer
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "monotonicMs"),
                         v8::Function::New(context, MonotonicMsCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && timer
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "sleep"),
                         v8::Function::New(context, SleepCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && timer
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "setTimeout"),
                         v8::Function::New(context, SetTimeoutCallback).ToLocalChecked())
                   .FromMaybe(false);

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Timer"), timer)
        .FromMaybe(false);
}

}  // namespace modules
