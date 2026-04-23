#include "modules/buffer_module.h"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return *utf8;
}

std::vector<uint8_t> ReadByteArray(v8::Isolate* isolate,
                                   v8::Local<v8::Context> context,
                                   v8::Local<v8::Value> value) {
    std::vector<uint8_t> out;
    if (!value->IsArray()) {
        return out;
    }

    v8::Local<v8::Array> array = value.As<v8::Array>();
    out.reserve(array->Length());
    for (uint32_t i = 0; i < array->Length(); ++i) {
        v8::Local<v8::Value> item;
        if (!array->Get(context, i).ToLocal(&item)) {
            out.push_back(0);
            continue;
        }
        const int32_t byte = item->Int32Value(context).FromMaybe(0);
        out.push_back(static_cast<uint8_t>(byte & 0xFF));
    }
    return out;
}

v8::Local<v8::Array> ToV8Array(v8::Isolate* isolate,
                               v8::Local<v8::Context> context,
                               const std::vector<uint8_t>& bytes) {
    v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(bytes.size()));
    for (uint32_t i = 0; i < bytes.size(); ++i) {
        (void)array->Set(context, i, v8::Integer::New(isolate, bytes[i])).FromMaybe(false);
    }
    return array;
}

void FromStringCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1) {
        args.GetReturnValue().Set(v8::Array::New(isolate));
        return;
    }

    const std::string text = ValueToString(isolate, args[0]);
    std::vector<uint8_t> bytes(text.begin(), text.end());
    args.GetReturnValue().Set(ToV8Array(isolate, context, bytes));
}

void ToStringCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsArray()) {
        args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(isolate, ""));
        return;
    }

    const std::vector<uint8_t> bytes = ReadByteArray(isolate, context, args[0]);
    const std::string text(bytes.begin(), bytes.end());
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(isolate, text.data(), v8::NewStringType::kNormal, text.size())
            .ToLocalChecked());
}

void ConcatCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    std::vector<uint8_t> out;
    if (args.Length() > 0 && args[0]->IsArray()) {
        std::vector<uint8_t> left = ReadByteArray(isolate, context, args[0]);
        out.insert(out.end(), left.begin(), left.end());
    }
    if (args.Length() > 1 && args[1]->IsArray()) {
        std::vector<uint8_t> right = ReadByteArray(isolate, context, args[1]);
        out.insert(out.end(), right.begin(), right.end());
    }

    args.GetReturnValue().Set(ToV8Array(isolate, context, out));
}

void LengthCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1) {
        args.GetReturnValue().Set(v8::Integer::New(isolate, 0));
        return;
    }

    if (args[0]->IsArray()) {
        args.GetReturnValue().Set(v8::Integer::New(isolate, args[0].As<v8::Array>()->Length()));
        return;
    }

    const std::string text = ValueToString(isolate, args[0]);
    args.GetReturnValue().Set(v8::Integer::New(isolate, static_cast<int>(text.size())));
}

void AllocCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    int size = 0;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        size = args[0].As<v8::Number>()->Value();
    }
    if (size < 0) {
        size = 0;
    }

    int fill = 0;
    if (args.Length() > 1 && args[1]->IsNumber()) {
        fill = args[1].As<v8::Number>()->Value();
    }

    std::vector<uint8_t> bytes(static_cast<size_t>(size), static_cast<uint8_t>(fill & 0xFF));
    args.GetReturnValue().Set(ToV8Array(isolate, context, bytes));
}

void ToHexCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsArray()) {
        args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(isolate, ""));
        return;
    }

    const std::vector<uint8_t> bytes = ReadByteArray(isolate, context, args[0]);
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (uint8_t b : bytes) {
        out << std::setw(2) << static_cast<int>(b);
    }
    const std::string hex = out.str();
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, hex.c_str()).ToLocalChecked());
}

}  // namespace

namespace modules {

bool RegisterBufferModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> buffer = v8::Object::New(isolate);
    bool ok = buffer
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "fromString"),
                        v8::Function::New(context, FromStringCallback).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && buffer
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "toString"),
                         v8::Function::New(context, ToStringCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && buffer
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "concat"),
                         v8::Function::New(context, ConcatCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && buffer
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "length"),
                         v8::Function::New(context, LengthCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && buffer
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "alloc"),
                         v8::Function::New(context, AllocCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && buffer
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "toHex"),
                         v8::Function::New(context, ToHexCallback).ToLocalChecked())
                   .FromMaybe(false);

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Buffer"), buffer)
        .FromMaybe(false);
}

}  // namespace modules
