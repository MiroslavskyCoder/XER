#include "flow_script_console.h"

#include <iostream>
#include <sstream>
#include <unordered_set>

namespace flow_script_detail {
namespace {

std::string HexByte(uint8_t b) {
    static const char* kHex = "0123456789abcdef";
    std::string out;
    out.push_back(kHex[(b >> 4) & 0x0F]);
    out.push_back(kHex[b & 0x0F]);
    return out;
}

std::string FormatStringForConsole(
    v8::Isolate* isolate,
    v8::Local<v8::String> str,
    bool top_level) {
    const int length = str->Length();
    std::vector<uint16_t> units(static_cast<size_t>(length));
    str->Write(
        isolate,
        units.data(),
        0,
        length,
        static_cast<int>(v8::String::WriteOptions::NO_NULL_TERMINATION));

    bool has_binary = false;
    for (uint16_t u : units) {
        if (u > 0xFF) {
            continue;
        }
        const uint8_t b = static_cast<uint8_t>(u & 0xFF);
        if (b < 0x20 || b == 0x7F) {
            has_binary = true;
            break;
        }
    }

    if (!has_binary) {
        const std::string plain = Utf8(isolate, str);
        if (top_level) {
            return plain;
        }
        return "\"" + plain + "\"";
    }

    std::ostringstream out;
    out << (top_level ? "b\"" : "\"b:");
    for (uint16_t u : units) {
        if (u <= 0xFF) {
            const uint8_t b = static_cast<uint8_t>(u & 0xFF);
            if (b >= 0x20 && b != 0x7F && b != '\\' && b != '"') {
                out << static_cast<char>(b);
            } else {
                out << "\\x" << HexByte(b);
            }
        } else {
            out << "\\u";
            const char* kHex = "0123456789abcdef";
            out << kHex[(u >> 12) & 0xF] << kHex[(u >> 8) & 0xF] << kHex[(u >> 4) & 0xF]
                << kHex[u & 0xF];
        }
    }
    out << '"';
    return out.str();
}

std::string Indent(int level) {
    return std::string(static_cast<size_t>(level * 2), ' ');
}

std::string FormatConsoleValue(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Value> value,
    int depth,
    int indent,
    bool top_level,
    std::unordered_set<int>* seen_hashes) {
    if (value->IsUndefined()) {
        return "undefined";
    }
    if (value->IsNull()) {
        return "null";
    }
    if (value->IsBoolean()) {
        return value->BooleanValue(isolate) ? "true" : "false";
    }
    if (value->IsNumber() || value->IsBigInt()) {
        return Utf8(isolate, value);
    }
    if (value->IsString()) {
        return FormatStringForConsole(isolate, value.As<v8::String>(), top_level);
    }
    if (value->IsFunction()) {
        return Utf8(isolate, value);
    }
    if (!value->IsObject()) {
        return Utf8(isolate, value);
    }

    if (depth <= 0) {
        return value->IsArray() ? "[Array]" : "[Object]";
    }

    v8::Local<v8::Object> obj = value.As<v8::Object>();
    const int identity_hash = obj->GetIdentityHash();
    if (identity_hash != 0 && seen_hashes->find(identity_hash) != seen_hashes->end()) {
        return "[Circular]";
    }
    if (identity_hash != 0) {
        seen_hashes->insert(identity_hash);
    }

    std::string out;
    if (value->IsArray()) {
        v8::Local<v8::Array> array = value.As<v8::Array>();
        const uint32_t length = array->Length();
        if (length == 0) {
            out = "[]";
        } else {
            out = "[\n";
            const uint32_t limit = length > 64 ? 64 : length;
            for (uint32_t i = 0; i < limit; ++i) {
                if (i > 0) {
                    out += ",\n";
                }
                v8::Local<v8::Value> element;
                if (!array->Get(context, i).ToLocal(&element)) {
                    out += Indent(indent + 1) + "<error>";
                    continue;
                }
                out += Indent(indent + 1)
                       + FormatConsoleValue(
                           isolate,
                           context,
                           element,
                           depth - 1,
                           indent + 1,
                           false,
                           seen_hashes);
            }
            if (limit < length) {
                out += ",\n" + Indent(indent + 1) + "... " + std::to_string(length - limit) + " more";
            }
            out += "\n" + Indent(indent) + "]";
        }
    } else {
        v8::Local<v8::Array> keys;
        if (!obj->GetOwnPropertyNames(context).ToLocal(&keys) || keys->Length() == 0) {
            out = "{}";
        } else {
            out = "{\n";
            const uint32_t length = keys->Length();
            const uint32_t limit = length > 64 ? 64 : length;
            for (uint32_t i = 0; i < limit; ++i) {
                if (i > 0) {
                    out += ",\n";
                }

                v8::Local<v8::Value> key;
                if (!keys->Get(context, i).ToLocal(&key)) {
                    out += Indent(indent + 1) + "<key-error>: <error>";
                    continue;
                }

                v8::Local<v8::Value> prop_value;
                if (!obj->Get(context, key).ToLocal(&prop_value)) {
                    out += Indent(indent + 1) + Utf8(isolate, key) + ": <error>";
                    continue;
                }

                out += Indent(indent + 1) + Utf8(isolate, key) + ": "
                       + FormatConsoleValue(
                           isolate,
                           context,
                           prop_value,
                           depth - 1,
                           indent + 1,
                           false,
                           seen_hashes);
            }

            if (limit < length) {
                out += ",\n" + Indent(indent + 1) + "... " + std::to_string(length - limit) + " more";
            }
            out += "\n" + Indent(indent) + "}";
        }
    }

    if (identity_hash != 0) {
        seen_hashes->erase(identity_hash);
    }
    return out;
}

}  // namespace

std::string Utf8(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return std::string(*utf8, static_cast<size_t>(utf8.length()));
}

std::string JoinArguments(v8::Isolate* isolate, const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    std::unordered_set<int> seen_hashes;

    std::string text;
    for (int i = 0; i < args.Length(); ++i) {
        if (i > 0) {
            text += " ";
        }

        text += FormatConsoleValue(isolate, context, args[i], 4, 0, true, &seen_hashes);
    }
    return text;
}

void ConsoleLogCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    std::cout << JoinArguments(isolate, args) << "\n";
}

void ConsoleErrorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    std::cerr << JoinArguments(isolate, args) << "\n";
}

}  // namespace flow_script_detail
