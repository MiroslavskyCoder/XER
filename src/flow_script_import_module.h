#pragma once

#include <v8.h>

namespace flow_script_detail {

void ImportModuleCallback(const v8::FunctionCallbackInfo<v8::Value>& args);

}  // namespace flow_script_detail
