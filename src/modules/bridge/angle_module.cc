#include "modules/bridge/angle_module.h"

#include "wrapper/angle/angle_engine_bridge.h"

namespace modules {

bool RegisterANGLEModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> module = v8::Object::New(isolate);
    bool ok = module
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "available"),
                        v8::Boolean::New(isolate, engine::bridge::angle::IsAvailable()))
                  .FromMaybe(false);

    v8::Local<v8::String> summary =
        v8::String::NewFromUtf8(isolate, engine::bridge::angle::Summary().c_str())
            .ToLocalChecked();
    ok = ok && module
                   ->Set(context, v8::String::NewFromUtf8Literal(isolate, "summary"), summary)
                   .FromMaybe(false);

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "ANGLE"), module)
        .FromMaybe(false);
}

}  // namespace modules