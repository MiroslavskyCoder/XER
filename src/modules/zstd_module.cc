#include "modules/zstd_module.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#ifndef ENGINE_HAS_ZSTD
#define ENGINE_HAS_ZSTD 0
#endif

#if ENGINE_HAS_ZSTD
#include <zstd.h>
#endif

namespace {

std::string ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return *utf8;
}

bool ReadBinaryStringBytes(v8::Isolate* isolate, v8::Local<v8::Value> value, std::vector<uint8_t>* out) {
    if (!value->IsString()) {
        return false;
    }

    v8::Local<v8::String> str = value.As<v8::String>();
    const int length = str->Length();
    std::vector<uint16_t> units(static_cast<size_t>(length));
    str->Write(isolate,
               units.data(),
               0,
               length,
               static_cast<int>(v8::String::WriteOptions::NO_NULL_TERMINATION));

    out->resize(static_cast<size_t>(length));
    for (int i = 0; i < length; ++i) {
        (*out)[static_cast<size_t>(i)] = static_cast<uint8_t>(units[static_cast<size_t>(i)] & 0xFF);
    }
    return true;
}

bool ReadBytesArg(v8::Isolate* isolate,
                  v8::Local<v8::Context> context,
                  v8::Local<v8::Value> value,
                  std::vector<uint8_t>* out) {
    out->clear();

    if (ReadBinaryStringBytes(isolate, value, out)) {
        return true;
    }

    if (value->IsArrayBufferView()) {
        v8::Local<v8::ArrayBufferView> view = value.As<v8::ArrayBufferView>();
        out->resize(view->ByteLength());
        view->CopyContents(out->data(), out->size());
        return true;
    }

    if (!value->IsArray()) {
        return false;
    }

    v8::Local<v8::Array> arr = value.As<v8::Array>();
    out->reserve(arr->Length());
    for (uint32_t i = 0; i < arr->Length(); ++i) {
        v8::Local<v8::Value> item;
        if (!arr->Get(context, i).ToLocal(&item) || !item->IsNumber()) {
            return false;
        }
        const int n = item.As<v8::Number>()->Value();
        out->push_back(static_cast<uint8_t>(n & 0xFF));
    }

    return true;
}

v8::Local<v8::String> ToBinaryString(v8::Isolate* isolate, const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        return v8::String::Empty(isolate);
    }
    return v8::String::NewFromOneByte(isolate,
                                      bytes.data(),
                                      v8::NewStringType::kNormal,
                                      static_cast<int>(bytes.size()))
        .ToLocalChecked();
}

#if ENGINE_HAS_ZSTD
void CompressCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "compress expects string input")));
        return;
    }

    int level = 3;
    if (args.Length() > 1 && args[1]->IsNumber()) {
        level = args[1].As<v8::Number>()->Value();
    }

    const std::string input = ValueToString(isolate, args[0]);
    const size_t max_size = ZSTD_compressBound(input.size());
    std::vector<uint8_t> output(max_size);

    const size_t result = ZSTD_compress(output.data(), output.size(), input.data(), input.size(), level);
    if (ZSTD_isError(result)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, ZSTD_getErrorName(result)).ToLocalChecked()));
        return;
    }

    output.resize(result);
    args.GetReturnValue().Set(ToBinaryString(isolate, output));
}

void DecompressCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "decompress expects binary string, Uint8Array or number[]")));
        return;
    }

    std::vector<uint8_t> input;
    if (!ReadBytesArg(isolate, context, args[0], &input)) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "decompress expects binary string, Uint8Array or number[]")));
        return;
    }

    const unsigned long long decompressed_size = ZSTD_getFrameContentSize(input.data(), input.size());
    if (decompressed_size == ZSTD_CONTENTSIZE_ERROR ||
        decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN ||
        decompressed_size > (64ULL * 1024ULL * 1024ULL)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "cannot determine zstd output size")));
        return;
    }

    std::vector<uint8_t> output(static_cast<size_t>(decompressed_size));
    const size_t result = ZSTD_decompress(output.data(), output.size(), input.data(), input.size());
    if (ZSTD_isError(result)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, ZSTD_getErrorName(result)).ToLocalChecked()));
        return;
    }

    const std::string decoded(reinterpret_cast<const char*>(output.data()), result);
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(isolate, decoded.data(), v8::NewStringType::kNormal, decoded.size())
            .ToLocalChecked());
}

void VersionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(args.GetIsolate(), ZSTD_versionString()).ToLocalChecked());
}
#else
void CompressCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetIsolate()->ThrowException(v8::Exception::Error(
        v8::String::NewFromUtf8Literal(args.GetIsolate(), "Zstd support is not available in this build")));
}

void DecompressCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetIsolate()->ThrowException(v8::Exception::Error(
        v8::String::NewFromUtf8Literal(args.GetIsolate(), "Zstd support is not available in this build")));
}

void VersionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(args.GetIsolate(), "unavailable"));
}
#endif

}  // namespace

namespace modules {

bool RegisterZstdModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> mod = v8::Object::New(isolate);
    bool ok = mod
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "compress"),
                        v8::Function::New(context, CompressCallback).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && mod
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "decompress"),
                         v8::Function::New(context, DecompressCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && mod
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "version"),
                         v8::Function::New(context, VersionCallback).ToLocalChecked())
                   .FromMaybe(false);

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Zstd"), mod)
        .FromMaybe(false);
}

}  // namespace modules
