#pragma once

#include <string>

#include <v8.h>

namespace modules::detail {

bool RegisterAiMlMethods(v8::Isolate* isolate,
				 v8::Local<v8::Context> context,
				 v8::Local<v8::Object> module,
				 std::string* error_out);

}  // namespace modules::detail