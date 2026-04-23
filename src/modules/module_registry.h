#pragma once

#include <v8.h>

#include <string>

namespace modules {

bool ImportModule(v8::Isolate* isolate,
                  v8::Local<v8::Context> context,
                  const std::string& module_name,
                  std::string* error_message);

}  // namespace modules
