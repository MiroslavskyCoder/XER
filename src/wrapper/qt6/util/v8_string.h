#pragma once

#include <string>

#include <v8.h>

#if ENGINE_HAS_QT6
#include <QString>
#endif

namespace qt6::util {

std::string V8ValueToStdString(v8::Isolate* isolate, v8::Local<v8::Value> value);

#if ENGINE_HAS_QT6
QString V8ValueToQString(v8::Isolate* isolate, v8::Local<v8::Value> value);
v8::Local<v8::String> QStringToV8String(v8::Isolate* isolate, const QString& value);
#endif

}  // namespace qt6::util