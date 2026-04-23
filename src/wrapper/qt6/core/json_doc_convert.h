#pragma once

#include <v8.h>

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#endif

namespace qt6::core::json_doc_detail {

#if ENGINE_HAS_QT6
v8::Local<v8::Value> QJsonValueToV8(v8::Isolate* isolate, const QJsonValue& value);
v8::Local<v8::Value> QJsonArrayToV8(v8::Isolate* isolate, const QJsonArray& array);
v8::Local<v8::Value> QJsonObjectToV8(v8::Isolate* isolate, const QJsonObject& object);

QJsonValue V8ToQJsonValue(v8::Isolate* isolate, v8::Local<v8::Value> value);
QJsonObject V8ObjectToQJson(v8::Isolate* isolate, v8::Local<v8::Object> object);
QJsonArray V8ArrayToQJson(v8::Isolate* isolate, v8::Local<v8::Array> array);
#endif

}  // namespace qt6::core::json_doc_detail
