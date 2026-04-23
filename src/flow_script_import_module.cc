#include "flow_script_import_module.h"

#include <string>

#include "modules/module_registry.h"

namespace flow_script_detail {

void ImportModuleCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "ImportModule expects module name string")));
        return;
    }

    v8::String::Utf8Value module_name_utf8(isolate, args[0]);
    if (*module_name_utf8 == nullptr) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Module name is not valid UTF-8")));
        return;
    }

    std::string error;
    if (!modules::ImportModule(isolate, context, *module_name_utf8, &error)) {
        if (error.empty()) {
            error = "Failed to import module";
        }
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, error.c_str()).ToLocalChecked()));
    }
}

}  // namespace flow_script_detail
