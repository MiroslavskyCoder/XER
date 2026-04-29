#pragma once

#include <string>

#include <v8.h>

#include "engine_params.h"

namespace Engine::V8Runtime {

v8::Isolate::CreateParams BuildCreateParams();
void ApplyMemorySettings(const EngineParams& params, std::string* warning_out);
void DestroyAllocator(v8::Isolate* isolate);

}  // namespace Engine::V8Runtime
