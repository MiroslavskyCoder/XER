#pragma once

// ---------------------------------------------------------------------------
// async_bridge — safe async C++ → V8 callback using v8::Isolate::RequestInterrupt
//
// Usage:
//   PostAsync(isolate, callback_fn, context,
//             []() { /* heavy work, returns data */ return result; },
//             [result](v8::Isolate* iso) {
//                 return /* build V8 value from result */;
//             });
//
// The work lambda runs on IOThreadPool::GetSharedInstance().
// The result factory lambda runs on V8's thread via RequestInterrupt.
// The JS callback receives (error, result) — Node-style.
// ---------------------------------------------------------------------------

#include <functional>
#include <memory>
#include <string>
#include <v8.h>
#include "async_io/io_thread_pool.h"

namespace Engine::Helper {

// ---------------------------------------------------------------------------
// AsyncState — holds the persistent callback + result factory
// Template parameter R = any copyable result type
// ---------------------------------------------------------------------------

struct AsyncStateBase {
    virtual ~AsyncStateBase() = default;
    virtual void Invoke(v8::Isolate* isolate) = 0;
};

template <typename R>
struct AsyncState : AsyncStateBase {
    v8::Persistent<v8::Function>  callback;
    v8::Persistent<v8::Context>   context;
    R                             result;
    std::string                   error;
    std::function<v8::Local<v8::Value>(v8::Isolate*, const R&)> result_factory;

    ~AsyncState() override {
        callback.Reset();
        context.Reset();
    }

    void Invoke(v8::Isolate* iso) override {
        v8::HandleScope hs(iso);
        auto ctx = context.Get(iso);
        auto cb  = callback.Get(iso);
        v8::Context::Scope cs(ctx);

        v8::Local<v8::Value> args[2];
        if (!error.empty()) {
            args[0] = v8::Exception::Error(
                v8::String::NewFromUtf8(iso, error.c_str()).ToLocalChecked());
            args[1] = v8::Undefined(iso);
        } else {
            args[0] = v8::Null(iso);
            args[1] = result_factory(iso, result);
        }

        cb->Call(ctx, ctx->Global(), 2, args).IsEmpty();  // ignore return
    }
};

// ---------------------------------------------------------------------------
// RequestInterrupt dispatch — called on V8 thread
// ---------------------------------------------------------------------------
void DispatchInterrupt(v8::Isolate* isolate, void* data);

// ---------------------------------------------------------------------------
// PostAsync<R>
//   iso       — V8 isolate (must remain alive during async op)
//   callback  — JS function to call when done: (err, result) => void
//   context   — current V8 context
//   work      — runs on thread pool, returns R (or sets error string)
//   result_fn — converts R to v8::Local<v8::Value> (on V8 thread)
// ---------------------------------------------------------------------------
template <typename R>
void PostAsync(
    v8::Isolate*            isolate,
    v8::Local<v8::Function> callback,
    v8::Local<v8::Context>  context,
    std::function<R(std::string& out_error)>                    work,
    std::function<v8::Local<v8::Value>(v8::Isolate*, const R&)> result_fn)
{
    auto* state = new AsyncState<R>();
    state->callback.Reset(isolate, callback);
    state->context.Reset(isolate, context);
    state->result_factory = std::move(result_fn);

    // Enqueue on IO thread pool
    ::IO::AsyncIO::IOThreadPool::GetSharedInstance().Enqueue([isolate, state, work]() {
        state->result = work(state->error);
        // Schedule back to V8 thread
        isolate->RequestInterrupt(DispatchInterrupt, state);
    });
}

// Specialised version for void work (fire-and-forget with status bool)
void PostAsyncVoid(
    v8::Isolate*            isolate,
    v8::Local<v8::Function> callback,
    v8::Local<v8::Context>  context,
    std::function<bool(std::string& out_error)> work);

}  // namespace Engine::Helper