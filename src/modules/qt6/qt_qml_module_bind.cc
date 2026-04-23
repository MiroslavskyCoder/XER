#include "modules/qt6/qt_qml_module.h"

#include "wrapper/qt6/v8/module_builder.h"

namespace modules::qt_qml_module_detail {

void QmlIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void QmlVersionCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void QmlCompileCb(const v8::FunctionCallbackInfo<v8::Value>& args);

}  // namespace modules::qt_qml_module_detail

namespace modules {

bool RegisterQtQmlModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope scope(isolate);
    using namespace qt6::v8bridge;

    v8::Local<v8::Object> mod = v8::Object::New(isolate);
    bool ok = true;
    ok = ok && SetMethod(isolate, context, mod, "isAvailable", qt_qml_module_detail::QmlIsAvailableCb);
    ok = ok && SetMethod(isolate, context, mod, "version", qt_qml_module_detail::QmlVersionCb);
    ok = ok && SetMethod(isolate, context, mod, "compile", qt_qml_module_detail::QmlCompileCb);
    if (!ok) return false;

    return ExportGlobalModule(isolate, context, "QtQml", mod);
}

}  // namespace modules
