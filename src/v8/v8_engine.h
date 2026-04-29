#pragma once

#include <string>

#include <v8.h>

#include "engine_params.h"

namespace Engine::V8Runtime {

v8::Isolate* CreateManagedIsolate(const EngineParams& params, std::string* warning_out = nullptr);
void DisposeManagedIsolate(v8::Isolate* isolate);

}  // namespace Engine::V8Runtime
