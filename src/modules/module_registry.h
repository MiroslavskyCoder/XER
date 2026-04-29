#pragma once

#include <string>
#include <vector>

#include <v8.h>

namespace modules {

std::string ResolveCanonicalModuleName(const std::string& module_name);
std::vector<std::string> ListModules();

bool ImportModule(v8::Isolate* isolate,
				  v8::Local<v8::Context> context,
				  const std::string& module_name,
				  std::string* error_out = nullptr);

}  // namespace modules