#pragma once

#include <string>

#include <v8.h>

namespace flux::console {

std::string RenderTable(v8::Isolate* isolate,
			v8::Local<v8::Context> context,
			v8::Local<v8::Value> input,
			v8::Local<v8::Value> columns_filter);

}  // namespace flux::console