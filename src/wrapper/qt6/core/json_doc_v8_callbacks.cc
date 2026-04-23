#include "wrapper/qt6/core/json_doc.h"

#include "wrapper/qt6/core/json_doc_convert.h"
#include "wrapper/qt6/v8/class_builder.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if ENGINE_HAS_QT6
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QString>
#endif

namespace qt6::core::json_doc_v8_detail {

using namespace qt6::v8bridge;

void JsonParse(const v8::FunctionCallbackInfo<v8::Value>& a) {
#if ENGINE_HAS_QT6
    auto iso = a.GetIsolate();
    if (a.Length() < 1 || !a[0]->IsString()) {
        ThrowTypeError(iso, "QtJson.parse(text: string)");
        return;
    }
    v8::String::Utf8Value u(iso, a[0]);
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(QByteArray(*u), &err);
    if (err.error != QJsonParseError::NoError) {
        ThrowError(iso, err.errorString().toUtf8().constData());
        return;
    }
    if (doc.isObject()) {
        a.GetReturnValue().Set(json_doc_detail::QJsonObjectToV8(iso, doc.object()));
    } else if (doc.isArray()) {
        a.GetReturnValue().Set(json_doc_detail::QJsonArrayToV8(iso, doc.array()));
    } else {
        a.GetReturnValue().Set(v8::Null(iso));
    }
#else
    ThrowError(a.GetIsolate(), "Qt6 unavailable");
#endif
}

void JsonStringify(const v8::FunctionCallbackInfo<v8::Value>& a) {
#if ENGINE_HAS_QT6
    auto iso = a.GetIsolate();
    if (a.Length() < 1) {
        ThrowTypeError(iso, "QtJson.stringify(value, indent?)");
        return;
    }
    int indent = 0;
    if (a.Length() >= 2 && a[1]->IsNumber()) {
        indent = a[1]->Int32Value(iso->GetCurrentContext()).FromMaybe(0);
    }

    QJsonValue qv = json_doc_detail::V8ToQJsonValue(iso, a[0]);
    QJsonDocument doc;
    if (qv.isObject()) {
        doc = QJsonDocument(qv.toObject());
    } else if (qv.isArray()) {
        doc = QJsonDocument(qv.toArray());
    }

    auto fmt = indent > 0 ? QJsonDocument::Indented : QJsonDocument::Compact;
    QByteArray bytes = doc.toJson(fmt);
    a.GetReturnValue().Set(v8::String::NewFromUtf8(iso, bytes.constData()).ToLocalChecked());
#else
    ThrowError(a.GetIsolate(), "Qt6 unavailable");
#endif
}

void JsonParseFile(const v8::FunctionCallbackInfo<v8::Value>& a) {
#if ENGINE_HAS_QT6
    auto iso = a.GetIsolate();
    if (a.Length() < 1 || !a[0]->IsString()) {
        ThrowTypeError(iso, "QtJson.parseFile(path: string)");
        return;
    }
    v8::String::Utf8Value u(iso, a[0]);
    QFile f(QString::fromUtf8(*u));
    if (!f.open(QIODevice::ReadOnly)) {
        ThrowError(iso, std::string("Cannot open: ") + *u);
        return;
    }
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        ThrowError(iso, err.errorString().toUtf8().constData());
        return;
    }
    if (doc.isObject()) {
        a.GetReturnValue().Set(json_doc_detail::QJsonObjectToV8(iso, doc.object()));
    } else if (doc.isArray()) {
        a.GetReturnValue().Set(json_doc_detail::QJsonArrayToV8(iso, doc.array()));
    } else {
        a.GetReturnValue().Set(v8::Null(iso));
    }
#else
    ThrowError(a.GetIsolate(), "Qt6 unavailable");
#endif
}

void JsonWriteFile(const v8::FunctionCallbackInfo<v8::Value>& a) {
#if ENGINE_HAS_QT6
    auto iso = a.GetIsolate();
    if (a.Length() < 2) {
        ThrowTypeError(iso, "QtJson.writeFile(value, path, indent?)");
        return;
    }
    int indent = a.Length() >= 3 ? a[2]->Int32Value(iso->GetCurrentContext()).FromMaybe(0) : 2;

    QJsonValue qv = json_doc_detail::V8ToQJsonValue(iso, a[0]);
    QJsonDocument doc;
    if (qv.isObject()) {
        doc = QJsonDocument(qv.toObject());
    } else if (qv.isArray()) {
        doc = QJsonDocument(qv.toArray());
    }

    auto fmt = indent > 0 ? QJsonDocument::Indented : QJsonDocument::Compact;
    v8::String::Utf8Value path(iso, a[1]);
    QFile f(QString::fromUtf8(*path));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        ThrowError(iso, std::string("Cannot write: ") + *path);
        return;
    }
    f.write(doc.toJson(fmt));
    a.GetReturnValue().Set(v8::Boolean::New(iso, true));
#else
    ThrowError(a.GetIsolate(), "Qt6 unavailable");
#endif
}

void JsonIsValid(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto iso = a.GetIsolate();
    if (a.Length() < 1 || !a[0]->IsString()) {
        a.GetReturnValue().Set(v8::Boolean::New(iso, false));
        return;
    }
#if ENGINE_HAS_QT6
    v8::String::Utf8Value u(iso, a[0]);
    QJsonParseError err;
    QJsonDocument::fromJson(QByteArray(*u), &err);
    a.GetReturnValue().Set(v8::Boolean::New(iso, err.error == QJsonParseError::NoError));
#else
    a.GetReturnValue().Set(v8::Boolean::New(iso, false));
#endif
}

}  // namespace qt6::core::json_doc_v8_detail
