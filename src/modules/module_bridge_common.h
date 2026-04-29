#pragma once

#include "modules/module_common.h"

#include <string>

namespace modules::detail {

bool SetBridgeStatusProperties(v8::Isolate* isolate,
			       v8::Local<v8::Context> context,
			       v8::Local<v8::Object> module,
			       const char* name,
			       bool available,
			       const std::string& summary,
			       std::string* error_out);

bool BuildBridgeStatusModule(v8::Isolate* isolate,
			     v8::Local<v8::Context> context,
			     const char* name,
			     bool available,
			     const std::string& summary,
			     v8::Local<v8::Object>* module_out,
			     std::string* error_out);

}  // namespace modules::detail