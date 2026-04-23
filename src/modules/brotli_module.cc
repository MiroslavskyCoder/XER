#include "modules/brotli_module.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#ifndef ENGINE_HAS_BROTLI
#define ENGINE_HAS_BROTLI 0
#endif

#if ENGINE_HAS_BROTLI
#include <brotli/decode.h>
#include <brotli/encode.h>
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

#if ENGINE_HAS_BROTLI
void CompressCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "compress expects string input")));
        return;
    }

    int quality = 5;
    if (args.Length() > 1 && args[1]->IsNumber()) {
        quality = args[1].As<v8::Number>()->Value();
        if (quality < 0) {
            quality = 0;
        }
        if (quality > 11) {
            quality = 11;
        }
    }

    const std::string input = ValueToString(isolate, args[0]);
    size_t out_size = BrotliEncoderMaxCompressedSize(input.size());
    std::vector<uint8_t> output(out_size);

    const BROTLI_BOOL ok = BrotliEncoderCompress(
        quality,
        BROTLI_DEFAULT_WINDOW,
        BROTLI_MODE_GENERIC,
        input.size(),
        reinterpret_cast<const uint8_t*>(input.data()),
        &out_size,
        output.data());

    if (ok == BROTLI_FALSE) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "brotli compress failed")));
        return;
    }

    output.resize(out_size);
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

    size_t out_size = input.size() * 4 + 1024;
    if (out_size < 1024) {
        out_size = 1024;
    }

    std::vector<uint8_t> output;
    BrotliDecoderResult result = BROTLI_DECODER_RESULT_ERROR;
    for (int attempt = 0; attempt < 8; ++attempt) {
        output.assign(out_size, 0);
        size_t decoded_size = out_size;
        result = BrotliDecoderDecompress(input.size(), input.data(), &decoded_size, output.data());
        if (result == BROTLI_DECODER_RESULT_SUCCESS) {
            output.resize(decoded_size);
            const std::string decoded(reinterpret_cast<const char*>(output.data()), output.size());
            args.GetReturnValue().Set(
                v8::String::NewFromUtf8(isolate, decoded.data(), v8::NewStringType::kNormal, decoded.size())
                    .ToLocalChecked());
            return;
        }

        if (result != BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
            break;
        }

        out_size *= 2;
        if (out_size > (64u * 1024u * 1024u)) {
            break;
        }
    }

    isolate->ThrowException(v8::Exception::Error(
        v8::String::NewFromUtf8Literal(isolate, "brotli decompress failed")));
}

void VersionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), static_cast<int>(BrotliDecoderVersion())));
}
#else
void CompressCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetIsolate()->ThrowException(v8::Exception::Error(
        v8::String::NewFromUtf8Literal(args.GetIsolate(), "Brotli support is not available in this build")));
}

void DecompressCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetIsolate()->ThrowException(v8::Exception::Error(
        v8::String::NewFromUtf8Literal(args.GetIsolate(), "Brotli support is not available in this build")));
}

void VersionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(args.GetIsolate(), "unavailable"));
}
#endif

}  // namespace

namespace modules {

bool RegisterBrotliModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
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
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Brotli"), mod)
        .FromMaybe(false);
}

}  // namespace modules
