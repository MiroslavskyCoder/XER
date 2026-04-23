#include "modules/qt6/qt_qml_module.h"

#include "wrapper/qt6/core/path.h"
#include "wrapper/qt6/util/v8_string.h"

#include <QCoreApplication>
#include <QUrl>

#if ENGINE_HAS_QT6_QML
#include <QQmlComponent>
#include <QQmlEngine>
#endif

#ifndef ENGINE_HAS_QT6_QML
#define ENGINE_HAS_QT6_QML 0
#endif

namespace modules::qt_qml_module_detail {

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

void QmlIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_QML == 1));
}

void QmlVersionCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(args.GetIsolate(), qt6::core::VersionString().c_str()).ToLocalChecked());
}

void QmlCompileCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "QtQml.compile(qmlText: string, baseUrl?: string)")));
        return;
    }

    v8::Local<v8::Object> result = v8::Object::New(isolate);

#if ENGINE_HAS_QT6_QML
    EnsureCoreApp();

    const std::string qml_text = qt6::util::V8ValueToStdString(isolate, args[0]);
    const std::string base = (args.Length() >= 2 && args[1]->IsString())
                           ? qt6::util::V8ValueToStdString(isolate, args[1])
                           : std::string("inline.qml");

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray::fromStdString(qml_text), QUrl::fromLocalFile(QString::fromStdString(base)));

    const bool ok = component.status() != QQmlComponent::Error;
    result->Set(context,
                v8::String::NewFromUtf8Literal(isolate, "ok"),
                v8::Boolean::New(isolate, ok)).Check();
    if (!ok) {
        const QString err = component.errorString();
        result->Set(context,
                    v8::String::NewFromUtf8Literal(isolate, "error"),
                    v8::String::NewFromUtf8(isolate, err.toStdString().c_str()).ToLocalChecked()).Check();
    }
#else
    result->Set(context,
                v8::String::NewFromUtf8Literal(isolate, "ok"),
                v8::Boolean::New(isolate, false)).Check();
    result->Set(context,
                v8::String::NewFromUtf8Literal(isolate, "error"),
                v8::String::NewFromUtf8Literal(isolate, "Qt6 QML support is not available in this build")).Check();
#endif

    args.GetReturnValue().Set(result);
}

}  // namespace modules::qt_qml_module_detail
