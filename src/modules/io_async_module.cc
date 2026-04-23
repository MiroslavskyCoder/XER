#include "modules/io_async_module.h"

#include "async_io/async_file_reader.h"
#include "async_io/async_file_writer.h"
#include "async_io/io_buffer_pool.h"
#include "async_io/io_cache_manager.h"

#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace {

v8::Local<v8::Object> GetOrCreateIONamespace(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::Local<v8::Value> io_candidate;
    if (context->Global()->Get(context, v8::String::NewFromUtf8Literal(isolate, "IO")).ToLocal(&io_candidate) &&
        io_candidate->IsObject()) {
        return io_candidate.As<v8::Object>();
    }

    v8::Local<v8::Object> io = v8::Object::New(isolate);
    (void)context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "IO"), io)
        .FromMaybe(false);
    return io;
}

std::string ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return *utf8;
}

IO::AsyncIO::IOCacheManager& SharedCache() {
    static IO::AsyncIO::IOCacheManager cache(16 * 1024 * 1024);
    return cache;
}

void PromisifiedInvokeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Data().IsEmpty() || !args.Data()->IsFunction()) {
        args.GetReturnValue().Set(v8::Undefined(isolate));
        return;
    }

    v8::Local<v8::Function> original = args.Data().As<v8::Function>();
    std::vector<v8::Local<v8::Value>> argv;
    argv.reserve(args.Length());
    for (int i = 0; i < args.Length(); ++i) {
        argv.push_back(args[i]);
    }

    v8::Local<v8::Value> result;
    if (!original->Call(context, context->Global(), static_cast<int>(argv.size()), argv.data()).ToLocal(&result)) {
        args.GetReturnValue().Set(v8::Undefined(isolate));
        return;
    }

    args.GetReturnValue().Set(result);
}

void PromisifyCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsFunction()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "promisify expects a function")));
        return;
    }

    v8::Local<v8::Function> wrapped =
        v8::Function::New(context, PromisifiedInvokeCallback, args[0]).ToLocalChecked();
    args.GetReturnValue().Set(wrapped);
}

void DelayCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    int ms = 0;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        ms = args[0].As<v8::Number>()->Value();
    }
    if (ms < 0) {
        ms = 0;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void RetryCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsFunction()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "retry expects a function")));
        return;
    }

    int attempts = 3;
    if (args.Length() > 1 && args[1]->IsNumber()) {
        attempts = args[1].As<v8::Number>()->Value();
    }
    if (attempts < 1) {
        attempts = 1;
    }

    int delay_ms = 0;
    if (args.Length() > 2 && args[2]->IsNumber()) {
        delay_ms = args[2].As<v8::Number>()->Value();
    }
    if (delay_ms < 0) {
        delay_ms = 0;
    }

    v8::Local<v8::Function> fn = args[0].As<v8::Function>();
    v8::TryCatch try_catch(isolate);

    for (int i = 0; i < attempts; ++i) {
        v8::Local<v8::Value> value;
        if (fn->Call(context, context->Global(), 0, nullptr).ToLocal(&value)) {
            args.GetReturnValue().Set(value);
            return;
        }

        if (i + 1 < attempts && delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    }

    if (try_catch.HasCaught()) {
        try_catch.ReThrow();
    }
}

void RunSeriesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsArray()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "runSeries expects array of functions")));
        return;
    }

    v8::Local<v8::Array> fns = args[0].As<v8::Array>();
    v8::Local<v8::Array> out = v8::Array::New(isolate, fns->Length());

    for (uint32_t i = 0; i < fns->Length(); ++i) {
        v8::Local<v8::Value> item;
        if (!fns->Get(context, i).ToLocal(&item) || !item->IsFunction()) {
            (void)out->Set(context, i, v8::Undefined(isolate)).FromMaybe(false);
            continue;
        }

        v8::Local<v8::Value> result;
        if (!item.As<v8::Function>()->Call(context, context->Global(), 0, nullptr).ToLocal(&result)) {
            return;
        }
        (void)out->Set(context, i, result).FromMaybe(false);
    }

    args.GetReturnValue().Set(out);
}

void ReadTextFastCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "readTextFast expects path string")));
        return;
    }

    const std::string path = ValueToString(isolate, args[0]);
    IO::AsyncIO::AsyncFileReader reader;
    if (!reader.OpenFile(path)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, reader.GetLastError().c_str()).ToLocalChecked()));
        return;
    }

    const int64_t size64 = reader.GetFileSize();
    if (size64 < 0) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to get file size")));
        return;
    }

    std::vector<uint8_t> bytes;
    if (!reader.ReadSync(static_cast<size_t>(size64), bytes)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, reader.GetLastError().c_str()).ToLocalChecked()));
        return;
    }

    const std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(isolate, text.data(), v8::NewStringType::kNormal, static_cast<int>(text.size()))
            .ToLocalChecked());
}

void WriteTextFastCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "writeTextFast expects path and text")));
        return;
    }

    const std::string path = ValueToString(isolate, args[0]);
    const std::string text = ValueToString(isolate, args[1]);
    const bool append = args.Length() > 2 && args[2]->BooleanValue(isolate);

    IO::AsyncIO::AsyncFileWriter writer;
    if (!writer.CreateFile(path, append)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, writer.GetLastError().c_str()).ToLocalChecked()));
        return;
    }

    const auto* data = reinterpret_cast<const uint8_t*>(text.data());
    if (!writer.WriteSync(data, text.size())) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, writer.GetLastError().c_str()).ToLocalChecked()));
        return;
    }

    args.GetReturnValue().Set(v8::Boolean::New(isolate, true));
}

void CachePutCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "cachePut expects key and text")));
        return;
    }

    const std::string key = ValueToString(isolate, args[0]);
    const std::string text = ValueToString(isolate, args[1]);
    std::vector<uint8_t> bytes(text.begin(), text.end());
    SharedCache().Put(key, std::move(bytes));
}

void CacheGetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "cacheGet expects key")));
        return;
    }

    std::vector<uint8_t> bytes;
    if (!SharedCache().Get(ValueToString(isolate, args[0]), bytes)) {
        args.GetReturnValue().Set(v8::Undefined(isolate));
        return;
    }

    const std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(isolate, text.data(), v8::NewStringType::kNormal, static_cast<int>(text.size()))
            .ToLocalChecked());
}

void CacheClearCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    SharedCache().Clear();
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
}

void CacheStatsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::Local<v8::Object> stats = v8::Object::New(isolate);
    (void)stats
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "entries"),
              v8::Number::New(isolate, static_cast<double>(SharedCache().GetEntryCount())))
        .FromMaybe(false);
    (void)stats
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "sizeBytes"),
              v8::Number::New(isolate, static_cast<double>(SharedCache().GetSizeBytes())))
        .FromMaybe(false);

    args.GetReturnValue().Set(stats);
}

void AllocBytesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    size_t size = 0;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        const double candidate = args[0].As<v8::Number>()->Value();
        if (candidate > 0) {
            size = static_cast<size_t>(candidate);
        }
    }

    const uint8_t fill = (args.Length() > 1 && args[1]->IsNumber())
                             ? static_cast<uint8_t>(args[1].As<v8::Number>()->Value())
                             : 0;

    auto buffer = IO::AsyncIO::IOBufferPool::Instance().Acquire(size);
    buffer->assign(size, fill);

    std::unique_ptr<v8::BackingStore> backing = v8::ArrayBuffer::NewBackingStore(isolate, size);
    if (size > 0) {
        std::memcpy(backing->Data(), buffer->data(), size);
    }

    v8::Local<v8::ArrayBuffer> ab = v8::ArrayBuffer::New(isolate, std::move(backing));
    v8::Local<v8::Uint8Array> view = v8::Uint8Array::New(ab, 0, size);
    args.GetReturnValue().Set(view);
}

void ClearBufferPoolCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    IO::AsyncIO::IOBufferPool::Instance().Clear();
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
}

}  // namespace

namespace modules {

bool RegisterIOAsyncModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> io = GetOrCreateIONamespace(isolate, context);
    v8::Local<v8::Object> async = v8::Object::New(isolate);

    bool ok = async
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "promisify"),
                        v8::Function::New(context, PromisifyCallback).ToLocalChecked())
                  .FromMaybe(false);
        ok = ok && async
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "delay"),
                     v8::Function::New(context, DelayCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && async
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "retry"),
                     v8::Function::New(context, RetryCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && async
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "runSeries"),
                     v8::Function::New(context, RunSeriesCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && async
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "readTextFast"),
                     v8::Function::New(context, ReadTextFastCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && async
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "writeTextFast"),
                     v8::Function::New(context, WriteTextFastCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && async
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "cachePut"),
                     v8::Function::New(context, CachePutCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && async
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "cacheGet"),
                     v8::Function::New(context, CacheGetCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && async
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "cacheClear"),
                     v8::Function::New(context, CacheClearCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && async
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "cacheStats"),
                     v8::Function::New(context, CacheStatsCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && async
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "allocBytes"),
                     v8::Function::New(context, AllocBytesCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && async
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "clearBufferPool"),
                     v8::Function::New(context, ClearBufferPoolCallback).ToLocalChecked())
                 .FromMaybe(false);

    ok = ok && io
                 ->Set(context,
                       v8::String::NewFromUtf8Literal(isolate, "Async"),
                       async)
                 .FromMaybe(false);

    return ok;
}

}  // namespace modules
