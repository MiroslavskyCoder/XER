#pragma once

#include <v8.h>

namespace flow_script_detail {

bool BindBuffer(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace flow_script_detail