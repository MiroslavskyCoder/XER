#include "modules/io_formating_module.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

namespace {

std::string ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return *utf8;
}

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

void ReplaceAll(std::string* text, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = text->find(from, pos)) != std::string::npos) {
        text->replace(pos, from.size(), to);
        pos += to.size();
    }
}

void SetOptionsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsObject()) {
        return;
    }

    (void)args.This()
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "__options"),
              args[0])
        .FromMaybe(false);
}

void ToDateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::Local<v8::Value> template_value;
    v8::Local<v8::Value> map_value;
    if (!args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "__template")).ToLocal(&template_value) ||
        !template_value->IsString() ||
        !args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "__map")).ToLocal(&map_value) ||
        !map_value->IsObject()) {
        args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(isolate, ""));
        return;
    }

    std::string output = ValueToString(isolate, template_value);
    v8::Local<v8::Object> map_obj = map_value.As<v8::Object>();

    v8::Local<v8::Array> keys;
    if (map_obj->GetOwnPropertyNames(context).ToLocal(&keys)) {
        for (uint32_t i = 0; i < keys->Length(); ++i) {
            v8::Local<v8::Value> key;
            if (!keys->Get(context, i).ToLocal(&key)) {
                continue;
            }
            v8::Local<v8::Value> val;
            if (!map_obj->Get(context, key).ToLocal(&val)) {
                continue;
            }

            const std::string key_s = ValueToString(isolate, key);
            const std::string val_s = ValueToString(isolate, val);
            ReplaceAll(&output, "%" + key_s + "%", val_s);
        }
    }

    v8::Local<v8::Value> options_value;
    if (args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "__options")).ToLocal(&options_value) &&
        options_value->IsObject()) {
        v8::Local<v8::Object> options_obj = options_value.As<v8::Object>();
        v8::Local<v8::Value> max_value;
        if (options_obj->Get(context, v8::String::NewFromUtf8Literal(isolate, "fmt_max")).ToLocal(&max_value) &&
            max_value->IsNumber()) {
            int max_len = max_value.As<v8::Number>()->Value();
            if (max_len >= 0 && static_cast<size_t>(max_len) < output.size()) {
                output.resize(static_cast<size_t>(max_len));
            }
        }
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, output.c_str()).ToLocalChecked());
}

void ObjectFormatCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() > 0 && args[0]->IsObject()) {
        args.GetReturnValue().Set(args[0]);
        return;
    }

    args.GetReturnValue().Set(v8::Object::New(isolate));
}

void OptionsFormatCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() > 0 && args[0]->IsObject()) {
        args.GetReturnValue().Set(args[0]);
        return;
    }

    args.GetReturnValue().Set(v8::Object::New(isolate));
}

void FormatingCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsObject()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Formating expects template and object format")));
        return;
    }

    v8::Local<v8::Object> formatter = v8::Object::New(isolate);
    bool ok = formatter
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__template"),
                        args[0])
                  .FromMaybe(false);
    ok = ok && formatter
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "__map"),
                         args[1])
                   .FromMaybe(false);
    ok = ok && formatter
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "__options"),
                         v8::Object::New(isolate))
                   .FromMaybe(false);
    ok = ok && formatter
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "setOptions"),
                         v8::Function::New(context, SetOptionsCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && formatter
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "toDate"),
                         v8::Function::New(context, ToDateCallback).ToLocalChecked())
                   .FromMaybe(false);

    if (!ok) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Unable to create formatting object")));
        return;
    }

    args.GetReturnValue().Set(formatter);
}

void JoinCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsArray()) {
        args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(isolate, ""));
        return;
    }

    const std::string separator = args.Length() > 1 ? ValueToString(isolate, args[1]) : std::string();
    v8::Local<v8::Array> input = args[0].As<v8::Array>();

    std::ostringstream out;
    for (uint32_t i = 0; i < input->Length(); ++i) {
        if (i > 0) {
            out << separator;
        }
        v8::Local<v8::Value> value;
        if (input->Get(context, i).ToLocal(&value)) {
            out << ValueToString(isolate, value);
        }
    }

    const std::string joined = out.str();
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, joined.c_str()).ToLocalChecked());
}

void UpperCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const std::string input = args.Length() > 0 ? ValueToString(args.GetIsolate(), args[0]) : std::string();
    std::string out = input;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), out.c_str()).ToLocalChecked());
}

void LowerCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const std::string input = args.Length() > 0 ? ValueToString(args.GetIsolate(), args[0]) : std::string();
    std::string out = input;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), out.c_str()).ToLocalChecked());
}

void TrimCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const std::string input = args.Length() > 0 ? ValueToString(args.GetIsolate(), args[0]) : std::string();

    size_t left = 0;
    while (left < input.size() && std::isspace(static_cast<unsigned char>(input[left])) != 0) {
        ++left;
    }
    size_t right = input.size();
    while (right > left && std::isspace(static_cast<unsigned char>(input[right - 1])) != 0) {
        --right;
    }

    const std::string out = input.substr(left, right - left);
    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), out.c_str()).ToLocalChecked());
}

void PadStartCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    std::string input = args.Length() > 0 ? ValueToString(isolate, args[0]) : std::string();
    int target_len = args.Length() > 1 && args[1]->IsNumber() ? args[1].As<v8::Number>()->Value() : 0;
    std::string fill = args.Length() > 2 ? ValueToString(isolate, args[2]) : " ";
    if (fill.empty()) {
        fill = " ";
    }

    while (static_cast<int>(input.size()) < target_len) {
        input.insert(0, fill);
    }
    if (static_cast<int>(input.size()) > target_len && target_len >= 0) {
        input = input.substr(input.size() - static_cast<size_t>(target_len));
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, input.c_str()).ToLocalChecked());
}

void PadEndCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    std::string input = args.Length() > 0 ? ValueToString(isolate, args[0]) : std::string();
    int target_len = args.Length() > 1 && args[1]->IsNumber() ? args[1].As<v8::Number>()->Value() : 0;
    std::string fill = args.Length() > 2 ? ValueToString(isolate, args[2]) : " ";
    if (fill.empty()) {
        fill = " ";
    }

    while (static_cast<int>(input.size()) < target_len) {
        input += fill;
    }
    if (static_cast<int>(input.size()) > target_len && target_len >= 0) {
        input.resize(static_cast<size_t>(target_len));
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, input.c_str()).ToLocalChecked());
}

void BytesHumanCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const double bytes = args.Length() > 0 && args[0]->IsNumber() ? args[0].As<v8::Number>()->Value() : 0.0;
    static const char* kUnits[] = {"B", "KB", "MB", "GB", "TB"};

    double value = bytes;
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }

    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(unit == 0 ? 0 : 2);
    out << value << " " << kUnits[unit];

    const std::string text = out.str();
    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), text.c_str()).ToLocalChecked());
}

}  // namespace

namespace modules {

bool RegisterIOFormatingModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> io = GetOrCreateIONamespace(isolate, context);

    bool ok = io
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "Formating"),
                        v8::Function::New(context, FormatingCallback).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && io
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "ObjectFormat"),
                         v8::Function::New(context, ObjectFormatCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && io
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "OptionsFormat"),
                         v8::Function::New(context, OptionsFormatCallback).ToLocalChecked())
                   .FromMaybe(false);
        ok = ok && io
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Join"),
                     v8::Function::New(context, JoinCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && io
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Upper"),
                     v8::Function::New(context, UpperCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && io
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Lower"),
                     v8::Function::New(context, LowerCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && io
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Trim"),
                     v8::Function::New(context, TrimCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && io
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "PadStart"),
                     v8::Function::New(context, PadStartCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && io
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "PadEnd"),
                     v8::Function::New(context, PadEndCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && io
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "BytesHuman"),
                     v8::Function::New(context, BytesHumanCallback).ToLocalChecked())
                 .FromMaybe(false);

    return ok;
}

}  // namespace modules
