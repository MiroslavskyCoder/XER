#include "modules/provider_module.h"

#include "runtime_live.h"

namespace modules {

bool RegisterProviderModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> provider = v8::Object::New(isolate);
    v8::Local<v8::String> provider_value =
        v8::String::NewFromUtf8(isolate, RuntimeLive::kProviderGPUToolkit).ToLocalChecked();
    bool ok = provider
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "GPUToolkit"),
                        provider_value)
                  .FromMaybe(false);
    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Provider"), provider)
        .FromMaybe(false);
}

}  // namespace modules
