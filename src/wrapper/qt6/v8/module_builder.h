#pragma once

#include <v8.h>

namespace qt6::v8bridge {

bool SetMethod(v8::Isolate* isolate,
               v8::Local<v8::Context> context,
               v8::Local<v8::Object> object,
               const char* name,
               v8::FunctionCallback callback);

bool ExportGlobalModule(v8::Isolate* isolate,
                        v8::Local<v8::Context> context,
                        const char* name,
                        v8::Local<v8::Object> module);

}  // namespace qt6::v8bridge