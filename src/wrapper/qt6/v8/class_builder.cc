#include "wrapper/qt6/v8/class_builder.h"

namespace qt6::v8bridge {

v8::Local<v8::FunctionTemplate> MakeClass(
    v8::Isolate*                     isolate,
    const char*                      class_name,
    v8::FunctionCallback             constructor,
    const std::vector<MethodDef>&    instance_methods,
    const std::vector<MethodDef>&    static_methods,
    const std::vector<AccessorDef>&  accessors,
    int                              internal_fields)
{
    v8::EscapableHandleScope scope(isolate);

    auto tpl = v8::FunctionTemplate::New(isolate, constructor);
    tpl->SetClassName(ToV8Str(isolate, class_name));
    tpl->InstanceTemplate()->SetInternalFieldCount(internal_fields);

    // Instance methods → prototype
    auto proto = tpl->PrototypeTemplate();
    for (const auto& m : instance_methods) {
        proto->Set(ToV8Str(isolate, m.name),
                   v8::FunctionTemplate::New(isolate, m.fn));
    }

    // Static methods → constructor itself
    for (const auto& m : static_methods) {
        tpl->Set(ToV8Str(isolate, m.name),
                 v8::FunctionTemplate::New(isolate, m.fn));
    }

    // Accessors (getters/setters) → instance template
    auto inst = tpl->InstanceTemplate();
    for (const auto& a : accessors) {
        inst->SetAccessor(ToV8Str(isolate, a.name), a.getter, a.setter);
    }

    return scope.Escape(tpl);
}

bool ExportClass(v8::Isolate*                   isolate,
                 v8::Local<v8::Context>          context,
                 const char*                     class_name,
                 v8::Local<v8::FunctionTemplate> tpl)
{
    auto maybe_fn = tpl->GetFunction(context);
    if (maybe_fn.IsEmpty()) return false;
    auto fn = maybe_fn.ToLocalChecked();
    return context->Global()
        ->Set(context, ToV8Str(isolate, class_name), fn)
        .FromMaybe(false);
}

}  // namespace qt6::v8bridge
