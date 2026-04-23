#include "modules/zlib_module.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <memory>

#include <zlib.h>

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

void CompressImpl(const v8::FunctionCallbackInfo<v8::Value>& args, bool use_deflate) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "compress expects string input")));
        return;
    }

    const std::string input = ValueToString(isolate, args[0]);
    int level = Z_DEFAULT_COMPRESSION;
    if (args.Length() > 1 && args[1]->IsNumber()) {
        level = args[1].As<v8::Number>()->Value();
        if (level < 0) {
            level = 0;
        }
        if (level > 9) {
            level = 9;
        }
    }

    z_stream stream{};
    const int window_bits = use_deflate ? MAX_WBITS : (MAX_WBITS + 16);
    if (deflateInit2(&stream, level, Z_DEFLATED, window_bits, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "zlib init failed")));
        return;
    }

    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
    stream.avail_in = static_cast<uInt>(input.size());

    std::vector<uint8_t> out;
    uint8_t buffer[16384];
    int ret = Z_OK;
    do {
        stream.next_out = buffer;
        stream.avail_out = sizeof(buffer);
        ret = deflate(&stream, Z_FINISH);
        const size_t produced = sizeof(buffer) - stream.avail_out;
        out.insert(out.end(), buffer, buffer + produced);
    } while (ret == Z_OK);

    deflateEnd(&stream);

    if (ret != Z_STREAM_END) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "zlib compress failed")));
        return;
    }

    args.GetReturnValue().Set(ToBinaryString(isolate, out));
}

void DecompressImpl(const v8::FunctionCallbackInfo<v8::Value>& args, bool use_inflate) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
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

    z_stream stream{};
    const int window_bits = use_inflate ? MAX_WBITS : (MAX_WBITS + 16);
    if (inflateInit2(&stream, window_bits) != Z_OK) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "zlib init failed")));
        return;
    }

    stream.next_in = input.empty() ? nullptr : reinterpret_cast<Bytef*>(input.data());
    stream.avail_in = static_cast<uInt>(input.size());

    std::vector<uint8_t> out;
    uint8_t buffer[16384];
    int ret = Z_OK;
    do {
        stream.next_out = buffer;
        stream.avail_out = sizeof(buffer);
        ret = inflate(&stream, Z_NO_FLUSH);
        const size_t produced = sizeof(buffer) - stream.avail_out;
        out.insert(out.end(), buffer, buffer + produced);
    } while (ret == Z_OK);

    inflateEnd(&stream);

    if (ret != Z_STREAM_END) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "zlib decompress failed")));
        return;
    }

    const std::string decoded(reinterpret_cast<const char*>(out.data()), out.size());
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(isolate, decoded.data(), v8::NewStringType::kNormal, decoded.size())
            .ToLocalChecked());
}

void CompressCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    CompressImpl(args, false);
}

void DecompressCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    DecompressImpl(args, false);
}

void DeflateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    CompressImpl(args, true);
}

void InflateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    DecompressImpl(args, true);
}

void VersionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), zlibVersion()).ToLocalChecked());
}

}  // namespace

namespace modules {

bool RegisterZlibModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
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
                         v8::String::NewFromUtf8Literal(isolate, "deflate"),
                         v8::Function::New(context, DeflateCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && mod
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "inflate"),
                         v8::Function::New(context, InflateCallback).ToLocalChecked())
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
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Zlib"), mod)
        .FromMaybe(false);
}

}  // namespace modules
