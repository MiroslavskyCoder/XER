#include "modules/qt6/qt_xml_module.h"

#include "wrapper/qt6/xml/reader.h"
#include "wrapper/qt6/util/v8_string.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

namespace modules::qt_xml_module_detail {

v8::Local<v8::Value> XmlElementToV8(v8::Isolate* iso,
                                     v8::Local<v8::Context> ctx,
                                     const qt6::xml::XmlElement& el) {
    v8::Local<v8::Object> obj = v8::Object::New(iso);

    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "name"),
             v8::String::NewFromUtf8(iso, el.name.c_str()).ToLocalChecked()).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "text"),
             v8::String::NewFromUtf8(iso, el.text.c_str()).ToLocalChecked()).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "ns"),
             v8::String::NewFromUtf8(iso, el.ns_uri.c_str()).ToLocalChecked()).Check();

    v8::Local<v8::Object> attrs_obj = v8::Object::New(iso);
    for (const auto& attr : el.attrs) {
        attrs_obj->Set(ctx,
            v8::String::NewFromUtf8(iso, attr.name.c_str()).ToLocalChecked(),
            v8::String::NewFromUtf8(iso, attr.value.c_str()).ToLocalChecked()).Check();
    }
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "attrs"), attrs_obj).Check();

    v8::Local<v8::Array> children = v8::Array::New(iso, static_cast<int>(el.children.size()));
    for (size_t i = 0; i < el.children.size(); ++i) {
        children->Set(ctx, static_cast<uint32_t>(i),
                      XmlElementToV8(iso, ctx, el.children[i])).Check();
    }
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "children"), children).Check();

    return obj;
}

void XmlParseCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    v8::Local<v8::Context> ctx = iso->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) {
        iso->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(iso, "Xml.parse(xmlText: string)")));
        return;
    }
    auto text   = qt6::util::V8ValueToStdString(iso, args[0]);
    auto result = qt6::xml::ParseXml(text);

    v8::Local<v8::Object> obj = v8::Object::New(iso);
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "ok"),
             v8::Boolean::New(iso, result.ok)).Check();
    if (!result.ok) {
        obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "error"),
                 v8::String::NewFromUtf8(iso, result.error.c_str()).ToLocalChecked()).Check();
    } else {
        obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "root"),
                 XmlElementToV8(iso, ctx, result.root)).Check();
    }
    args.GetReturnValue().Set(obj);
}

void XmlParseFileCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    v8::Local<v8::Context> ctx = iso->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) {
        iso->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(iso, "Xml.parseFile(path: string)")));
        return;
    }
    auto result = qt6::xml::ParseXmlFile(qt6::util::V8ValueToStdString(iso, args[0]));

    v8::Local<v8::Object> obj = v8::Object::New(iso);
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "ok"),
             v8::Boolean::New(iso, result.ok)).Check();
    if (!result.ok) {
        obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "error"),
                 v8::String::NewFromUtf8(iso, result.error.c_str()).ToLocalChecked()).Check();
        obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "line"),
                 v8::Integer::New(iso, result.error_line)).Check();
        obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "column"),
                 v8::Integer::New(iso, result.error_column)).Check();
    } else {
        obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "root"),
                 XmlElementToV8(iso, ctx, result.root)).Check();
    }
    args.GetReturnValue().Set(obj);
}

void XmlIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(
        v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6 == 1));
}

}  // namespace modules::qt_xml_module_detail
