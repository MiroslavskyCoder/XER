#include "modules/util_module.h"

#include <cmath>
#include <iostream>
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

std::string Base64Encode(const std::string& input) {
    static const char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);

    int val = 0;
    int valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            output.push_back(kAlphabet[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6) {
        output.push_back(kAlphabet[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (output.size() % 4 != 0) {
        output.push_back('=');
    }

    return output;
}

void ConvertToTypeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    std::string target_type;
    if (!args.Data().IsEmpty() && args.Data()->IsString()) {
        target_type = ValueToString(isolate, args.Data());
    }

    if (args.Length() < 1) {
        args.GetReturnValue().Set(v8::Undefined(isolate));
        return;
    }

    const std::string value = ValueToString(isolate, args[0]);

    try {
        if (target_type == "int") {
            int result = static_cast<int>(std::stod(value));
            args.GetReturnValue().Set(v8::Integer::New(isolate, result));
            return;
        }

        if (target_type == "float" || target_type == "double") {
            double result = std::stod(value);
            args.GetReturnValue().Set(v8::Number::New(isolate, result));
            return;
        }
    } catch (...) {
        args.GetReturnValue().Set(v8::Undefined(isolate));
        return;
    }

    args.GetReturnValue().Set(args[0]);
}

void StringToBase64Callback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "stringToBase64 expects one argument")));
        return;
    }

    const std::string input = ValueToString(isolate, args[0]);
    const std::string encoded = Base64Encode(input);
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, encoded.c_str()).ToLocalChecked());
}

void B64LengthCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1) {
        args.GetReturnValue().Set(v8::Integer::New(isolate, 0));
        return;
    }

    const std::string encoded = ValueToString(isolate, args[0]);
    args.GetReturnValue().Set(v8::Integer::New(isolate, static_cast<int>(encoded.size())));
}

void TraceOutCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1) {
        return;
    }

    std::cout << ValueToString(isolate, args[0]) << "\n";
}

void ToCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "to expects target type string")));
        return;
    }

    v8::Local<v8::Function> converter =
        v8::Function::New(context, ConvertToTypeCallback, args[0]).ToLocalChecked();
    args.GetReturnValue().Set(converter);
}

}  // namespace

namespace modules {

bool RegisterUtilModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> util = v8::Object::New(isolate);
    bool ok = util
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "stringToBase64"),
                        v8::Function::New(context, StringToBase64Callback).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && util
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "b64Length"),
                         v8::Function::New(context, B64LengthCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && util
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "traceOut"),
                         v8::Function::New(context, TraceOutCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && util
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "to"),
                         v8::Function::New(context, ToCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && util
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "invoke"),
                         v8::Boolean::New(isolate, false))
                   .FromMaybe(false);

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Util"), util)
        .FromMaybe(false);
}

}  // namespace modules
