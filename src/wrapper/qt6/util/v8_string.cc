#include "wrapper/qt6/util/v8_string.h"

namespace qt6::util {

std::string V8ValueToStdString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return *utf8;
}

#if ENGINE_HAS_QT6
QString V8ValueToQString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    return QString::fromUtf8(V8ValueToStdString(isolate, value).c_str());
}

v8::Local<v8::String> QStringToV8String(v8::Isolate* isolate, const QString& value) {
    const QByteArray utf8 = value.toUtf8();
    return v8::String::NewFromUtf8(isolate, utf8.constData()).ToLocalChecked();
}
#endif

}  // namespace qt6::util