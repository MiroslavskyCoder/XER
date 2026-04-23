#include "modules/qt6/qt_xml_module.h"

#include "wrapper/qt6/v8/module_builder.h"

namespace modules::qt_xml_module_detail {

void XmlIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void XmlParseCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void XmlParseFileCb(const v8::FunctionCallbackInfo<v8::Value>& args);

}  // namespace modules::qt_xml_module_detail

namespace modules {

bool RegisterQtXmlModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope scope(isolate);
    using namespace qt6::v8bridge;

    v8::Local<v8::Object> mod = v8::Object::New(isolate);
    bool ok = true;
    ok = ok && SetMethod(isolate, context, mod, "isAvailable", qt_xml_module_detail::XmlIsAvailableCb);
    ok = ok && SetMethod(isolate, context, mod, "parse",       qt_xml_module_detail::XmlParseCb);
    ok = ok && SetMethod(isolate, context, mod, "parseFile",   qt_xml_module_detail::XmlParseFileCb);
    if (!ok) return false;

    return ExportGlobalModule(isolate, context, "QtXml", mod);
}

}  // namespace modules
