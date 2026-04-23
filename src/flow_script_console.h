#pragma once

#include <v8.h>

#include <cstdint>
#include <string>
#include <vector>

namespace flow_script_detail {

std::string Utf8(v8::Isolate* isolate, v8::Local<v8::Value> value);

std::string JoinArguments(v8::Isolate* isolate, const v8::FunctionCallbackInfo<v8::Value>& args);

void ConsoleLogCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void ConsoleErrorCallback(const v8::FunctionCallbackInfo<v8::Value>& args);

bool ValueToByteVector(
    v8::Local<v8::Context> context,
    v8::Local<v8::Value> value,
    std::vector<std::uint8_t>* out);

v8::Local<v8::Uint8Array> ByteVectorToUint8Array(
    v8::Isolate* isolate,
    const std::vector<std::uint8_t>& bytes);

}  // namespace flow_script_detail
