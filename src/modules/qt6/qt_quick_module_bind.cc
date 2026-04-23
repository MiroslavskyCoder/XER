#include "modules/qt6/qt_quick_module.h"

#include "wrapper/qt6/v8/module_builder.h"

namespace modules::qt_quick_module_detail {

void QuickIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void QuickVersionCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void QuickValidateItemCb(const v8::FunctionCallbackInfo<v8::Value>& args);

}  // namespace modules::qt_quick_module_detail

namespace modules {

bool RegisterQtQuickModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope scope(isolate);
    using namespace qt6::v8bridge;

    v8::Local<v8::Object> mod = v8::Object::New(isolate);
    bool ok = true;
    ok = ok && SetMethod(isolate, context, mod, "isAvailable", qt_quick_module_detail::QuickIsAvailableCb);
    ok = ok && SetMethod(isolate, context, mod, "version", qt_quick_module_detail::QuickVersionCb);
    ok = ok && SetMethod(isolate, context, mod, "validateItem", qt_quick_module_detail::QuickValidateItemCb);
    if (!ok) return false;

    return ExportGlobalModule(isolate, context, "QtQuick", mod);
}

}  // namespace modules
