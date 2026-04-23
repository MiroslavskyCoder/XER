#include "wrapper/qt6/core/json_doc.h"
#include "wrapper/qt6/core/json_doc_convert.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#endif

namespace qt6::core {

#if ENGINE_HAS_QT6
namespace json_doc_detail {

v8::Local<v8::Value> QJsonValueToV8(v8::Isolate* iso, const QJsonValue& v) {
    switch (v.type()) {
        case QJsonValue::Null:      return v8::Null(iso);
        case QJsonValue::Bool:      return v8::Boolean::New(iso, v.toBool());
        case QJsonValue::Double:    return v8::Number::New(iso, v.toDouble());
        case QJsonValue::String:
            return v8::String::NewFromUtf8(iso,
                v.toString().toUtf8().constData()).ToLocalChecked();
        case QJsonValue::Array:     return QJsonArrayToV8(iso, v.toArray());
        case QJsonValue::Object:    return QJsonObjectToV8(iso, v.toObject());
        case QJsonValue::Undefined: return v8::Undefined(iso);
        default: return v8::Null(iso);
    }
}

v8::Local<v8::Value> QJsonArrayToV8(v8::Isolate* iso, const QJsonArray& arr) {
    auto ctx   = iso->GetCurrentContext();
    auto v8arr = v8::Array::New(iso, arr.size());
    for (int i = 0; i < arr.size(); ++i)
        v8arr->Set(ctx, i, QJsonValueToV8(iso, arr[i])).Check();
    return v8arr;
}

v8::Local<v8::Value> QJsonObjectToV8(v8::Isolate* iso, const QJsonObject& obj) {
    auto ctx    = iso->GetCurrentContext();
    auto v8obj  = v8::Object::New(iso);
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        auto key = v8::String::NewFromUtf8(iso,
            it.key().toUtf8().constData()).ToLocalChecked();
        v8obj->Set(ctx, key, QJsonValueToV8(iso, it.value())).Check();
    }
    return v8obj;
}

QJsonValue V8ToQJsonValue(v8::Isolate* iso, v8::Local<v8::Value> val) {
    if (val.IsEmpty() || val->IsNull() || val->IsUndefined())
        return QJsonValue::Null;
    if (val->IsBoolean())  return QJsonValue(val->BooleanValue(iso));
    if (val->IsNumber())   return QJsonValue(val->NumberValue(iso->GetCurrentContext()).FromMaybe(0.0));
    if (val->IsString()) {
        v8::String::Utf8Value u(iso, val);
        return QJsonValue(QString::fromUtf8(*u ? *u : ""));
    }
    if (val->IsArray())    return QJsonValue(V8ArrayToQJson(iso, val.As<v8::Array>()));
    if (val->IsObject())   return QJsonValue(V8ObjectToQJson(iso, val.As<v8::Object>()));
    return QJsonValue::Null;
}

QJsonObject V8ObjectToQJson(v8::Isolate* iso, v8::Local<v8::Object> obj) {
    QJsonObject qobj;
    auto ctx = iso->GetCurrentContext();
    auto maybe_names = obj->GetOwnPropertyNames(ctx);
    if (maybe_names.IsEmpty()) return qobj;
    auto names = maybe_names.ToLocalChecked();
    for (uint32_t i = 0; i < names->Length(); ++i) {
        auto key_v8 = names->Get(ctx, i).ToLocalChecked();
        v8::String::Utf8Value key_str(iso, key_v8);
        auto val = obj->Get(ctx, key_v8).ToLocalChecked();
        qobj.insert(QString::fromUtf8(*key_str), V8ToQJsonValue(iso, val));
    }
    return qobj;
}

QJsonArray V8ArrayToQJson(v8::Isolate* iso, v8::Local<v8::Array> arr) {
    QJsonArray qarr;
    auto ctx = iso->GetCurrentContext();
    for (uint32_t i = 0; i < arr->Length(); ++i)
        qarr.append(V8ToQJsonValue(iso, arr->Get(ctx, i).ToLocalChecked()));
    return qarr;
}

}  // namespace json_doc_detail
#endif  // ENGINE_HAS_QT6

}  // namespace qt6::core
