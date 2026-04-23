#include "modules/qt6/qt_module.h"

#include "wrapper/qt6/v8/module_builder.h"

namespace modules::qt_module_detail {

void IsAvailableCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void VersionCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void HasCoreCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void HasGuiCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void HasWidgetsCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void HasWebEngineCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void HasWebViewCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void HasWebChannelCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void HasQmlCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void HasQuickCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void LoadCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void LoadAllCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void CleanPathCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void ToNativeSeparatorsCallback(const v8::FunctionCallbackInfo<v8::Value>& args);

}  // namespace modules::qt_module_detail

namespace modules {

bool RegisterQtModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> mod = v8::Object::New(isolate);
    bool ok = qt6::v8bridge::SetMethod(isolate, context, mod, "isAvailable", qt_module_detail::IsAvailableCallback);
    ok = ok && qt6::v8bridge::SetMethod(isolate, context, mod, "version", qt_module_detail::VersionCallback);
    ok = ok && qt6::v8bridge::SetMethod(isolate, context, mod, "hasCore", qt_module_detail::HasCoreCallback);
    ok = ok && qt6::v8bridge::SetMethod(isolate, context, mod, "hasGui", qt_module_detail::HasGuiCallback);
    ok = ok && qt6::v8bridge::SetMethod(isolate, context, mod, "hasWidgets", qt_module_detail::HasWidgetsCallback);
    ok = ok && qt6::v8bridge::SetMethod(isolate, context, mod, "hasWebEngine", qt_module_detail::HasWebEngineCallback);
    ok = ok && qt6::v8bridge::SetMethod(isolate, context, mod, "hasWebView", qt_module_detail::HasWebViewCallback);
    ok = ok && qt6::v8bridge::SetMethod(isolate, context, mod, "hasWebChannel", qt_module_detail::HasWebChannelCallback);
    ok = ok && qt6::v8bridge::SetMethod(isolate, context, mod, "hasQml", qt_module_detail::HasQmlCallback);
    ok = ok && qt6::v8bridge::SetMethod(isolate, context, mod, "hasQuick", qt_module_detail::HasQuickCallback);
    ok = ok && qt6::v8bridge::SetMethod(isolate, context, mod, "load", qt_module_detail::LoadCallback);
    ok = ok && qt6::v8bridge::SetMethod(isolate, context, mod, "loadAll", qt_module_detail::LoadAllCallback);
    ok = ok && qt6::v8bridge::SetMethod(isolate, context, mod, "cleanPath", qt_module_detail::CleanPathCallback);
    ok = ok && qt6::v8bridge::SetMethod(isolate, context, mod, "toNativeSeparators", qt_module_detail::ToNativeSeparatorsCallback);

    if (!ok) {
        return false;
    }

    return qt6::v8bridge::ExportGlobalModule(isolate, context, "Qt", mod);
}

}  // namespace modules
