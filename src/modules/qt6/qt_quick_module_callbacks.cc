#include "modules/qt6/qt_quick_module.h"

#include "wrapper/qt6/core/path.h"
#include "wrapper/qt6/util/v8_string.h"

#include <QCoreApplication>
#include <QUrl>

#if ENGINE_HAS_QT6_QML
#include <QQmlComponent>
#include <QQmlEngine>
#endif

#if ENGINE_HAS_QT6_QUICK
#include <QQuickItem>
#endif

#ifndef ENGINE_HAS_QT6_QML
#define ENGINE_HAS_QT6_QML 0
#endif

#ifndef ENGINE_HAS_QT6_QUICK
#define ENGINE_HAS_QT6_QUICK 0
#endif

namespace modules::qt_quick_module_detail {

namespace {
QCoreApplication* EnsureCoreApp() {
    if (auto* app = QCoreApplication::instance()) {
        return app;
    }

    static int argc = 1;
    static char app_name[] = "enginebuilder";
    static char* argv[] = { app_name, nullptr };
    static QCoreApplication* created = new QCoreApplication(argc, argv);
    return created;
}
}  // namespace

void QuickIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const bool ok = (ENGINE_HAS_QT6_QML == 1) && (ENGINE_HAS_QT6_QUICK == 1);
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
}

void QuickVersionCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(args.GetIsolate(), qt6::core::VersionString().c_str()).ToLocalChecked());
}

void QuickValidateItemCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "QtQuick.validateItem(qmlText: string, baseUrl?: string)")));
        return;
    }

    v8::Local<v8::Object> result = v8::Object::New(isolate);

#if ENGINE_HAS_QT6_QML && ENGINE_HAS_QT6_QUICK
    EnsureCoreApp();

    const std::string qml_text = qt6::util::V8ValueToStdString(isolate, args[0]);
    const std::string base = (args.Length() >= 2 && args[1]->IsString())
                           ? qt6::util::V8ValueToStdString(isolate, args[1])
                           : std::string("inline_quick.qml");

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray::fromStdString(qml_text), QUrl::fromLocalFile(QString::fromStdString(base)));

    bool ok = component.status() != QQmlComponent::Error;
    bool is_item = false;
    std::string error;

    QObject* created_object = nullptr;
    if (ok) {
        created_object = component.create();
        if (created_object == nullptr) {
            ok = false;
            error = "Failed to create QML object";
        }
    }

    if (created_object != nullptr) {
#if ENGINE_HAS_QT6_QUICK
        is_item = qobject_cast<QQuickItem*>(created_object) != nullptr;
#endif
        created_object->deleteLater();
    }

    if (!ok && error.empty()) {
        error = component.errorString().toStdString();
    }

    result->Set(context,
                v8::String::NewFromUtf8Literal(isolate, "ok"),
                v8::Boolean::New(isolate, ok)).Check();
    result->Set(context,
                v8::String::NewFromUtf8Literal(isolate, "isItem"),
                v8::Boolean::New(isolate, is_item)).Check();
    if (!error.empty()) {
        result->Set(context,
                    v8::String::NewFromUtf8Literal(isolate, "error"),
                    v8::String::NewFromUtf8(isolate, error.c_str()).ToLocalChecked()).Check();
    }
#else
    result->Set(context,
                v8::String::NewFromUtf8Literal(isolate, "ok"),
                v8::Boolean::New(isolate, false)).Check();
    result->Set(context,
                v8::String::NewFromUtf8Literal(isolate, "isItem"),
                v8::Boolean::New(isolate, false)).Check();
    result->Set(context,
                v8::String::NewFromUtf8Literal(isolate, "error"),
                v8::String::NewFromUtf8Literal(isolate, "Qt6 Quick/QML support is not available in this build")).Check();
#endif

    args.GetReturnValue().Set(result);
}

}  // namespace modules::qt_quick_module_detail
