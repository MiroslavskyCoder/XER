#include "wrapper/qt6/v8/module_builder.h"

namespace qt6::v8bridge {

bool SetMethod(v8::Isolate* isolate,
               v8::Local<v8::Context> context,
               v8::Local<v8::Object> object,
               const char* name,
               v8::FunctionCallback callback) {
    return object
        ->Set(context,
              v8::String::NewFromUtf8(isolate, name).ToLocalChecked(),
              v8::Function::New(context, callback).ToLocalChecked())
        .FromMaybe(false);
}

bool ExportGlobalModule(v8::Isolate* isolate,
                        v8::Local<v8::Context> context,
                        const char* name,
                        v8::Local<v8::Object> module) {
    return context->Global()
        ->Set(context, v8::String::NewFromUtf8(isolate, name).ToLocalChecked(), module)
        .FromMaybe(false);
}

}  // namespace qt6::v8bridge