#pragma once

#include <v8.h>

namespace modules {

bool RegisterNetworkModule(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace modules
