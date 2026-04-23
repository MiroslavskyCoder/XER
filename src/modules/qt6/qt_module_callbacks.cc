#include "modules/qt6/qt_module.h"

#include "modules/qt6/qt_core_module.h"
#include "modules/qt6/qt_gui_module.h"
#include "modules/qt6/qt_pdf_module.h"
#include "modules/qt6/qt_qml_module.h"
#include "modules/qt6/qt_quick_module.h"
#include "modules/qt6/qt_web_module.h"
#include "modules/qt6/qt_xml_module.h"

#include "wrapper/qt6/core/path.h"
#include "wrapper/qt6/util/v8_string.h"

#ifndef ENGINE_HAS_QT6_CORE
#define ENGINE_HAS_QT6_CORE 0
#endif

#ifndef ENGINE_HAS_QT6_GUI
#define ENGINE_HAS_QT6_GUI 0
#endif

#ifndef ENGINE_HAS_QT6_WIDGETS
#define ENGINE_HAS_QT6_WIDGETS 0
#endif

#ifndef ENGINE_HAS_QT6_WEBVIEW
#define ENGINE_HAS_QT6_WEBVIEW 0
#endif

#ifndef ENGINE_HAS_QT6_WEBCHANNEL
#define ENGINE_HAS_QT6_WEBCHANNEL 0
#endif

#ifndef ENGINE_HAS_QT6_WEBENGINE
#define ENGINE_HAS_QT6_WEBENGINE 0
#endif

#ifndef ENGINE_HAS_QT6_QML
#define ENGINE_HAS_QT6_QML 0
#endif

#ifndef ENGINE_HAS_QT6_QUICK
#define ENGINE_HAS_QT6_QUICK 0
#endif

namespace modules::qt_module_detail {

static bool LoadQtSubmodule(v8::Isolate* isolate,
                            v8::Local<v8::Context> context,
                            const std::string& name) {
    if (name == "Core" || name == "Qt/Core") return modules::RegisterQtCoreModule(isolate, context);
    if (name == "Gui" || name == "Qt/Gui") return modules::RegisterQtGuiModule(isolate, context);
    if (name == "Widgets" || name == "Qt/Widgets") return modules::RegisterQtGuiModule(isolate, context);
    if (name == "Application" || name == "Qt/Application") return modules::RegisterQtGuiModule(isolate, context);
    if (name == "Xml" || name == "Qt/Xml") return modules::RegisterQtXmlModule(isolate, context);
    if (name == "Pdf" || name == "Qt/Pdf") return modules::RegisterQtPdfModule(isolate, context);
    if (name == "Web" || name == "Qt/Web") return modules::RegisterQtWebModule(isolate, context);
    if (name == "WebEngine" || name == "Qt/WebEngine") return modules::RegisterQtWebModule(isolate, context);
    if (name == "WebView" || name == "Qt/WebView") return modules::RegisterQtWebModule(isolate, context);
    if (name == "WebChannel" || name == "Qt/WebChannel") return modules::RegisterQtWebModule(isolate, context);
    if (name == "Qml" || name == "Qt/Qml") return modules::RegisterQtQmlModule(isolate, context);
    if (name == "Quick" || name == "Qt/Quick") return modules::RegisterQtQuickModule(isolate, context);
    return false;
}

void IsAvailableCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), qt6::core::IsAvailable()));
}

void VersionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(args.GetIsolate(), qt6::core::VersionString().c_str()).ToLocalChecked());
}

void HasCoreCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_CORE == 1));
}

void HasGuiCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_GUI == 1));
}

void HasWidgetsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_WIDGETS == 1));
}

void HasWebEngineCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_WEBENGINE == 1));
}

void HasWebViewCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_WEBVIEW == 1));
}

void HasWebChannelCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_WEBCHANNEL == 1));
}

void HasQmlCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_QML == 1));
}

void HasQuickCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_QUICK == 1));
}

void LoadCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Qt.load(name: string)")));
        return;
    }

    const std::string name = qt6::util::V8ValueToStdString(isolate, args[0]);
    const bool ok = LoadQtSubmodule(isolate, context, name);
    if (!ok) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, ("Unknown Qt submodule: " + name).c_str()).ToLocalChecked()));
        return;
    }
    args.GetReturnValue().Set(v8::Boolean::New(isolate, true));
}

void LoadAllCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    auto context = isolate->GetCurrentContext();

    bool ok = true;
    ok = ok && modules::RegisterQtCoreModule(isolate, context);
    ok = ok && modules::RegisterQtGuiModule(isolate, context);
    ok = ok && modules::RegisterQtXmlModule(isolate, context);
    ok = ok && modules::RegisterQtPdfModule(isolate, context);
    ok = ok && modules::RegisterQtWebModule(isolate, context);
    ok = ok && modules::RegisterQtQmlModule(isolate, context);
    ok = ok && modules::RegisterQtQuickModule(isolate, context);

    if (!ok) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to register one or more Qt modules")));
        return;
    }
    args.GetReturnValue().Set(v8::Boolean::New(isolate, true));
}

void CleanPathCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "cleanPath expects path string")));
        return;
    }

    if (!qt6::core::IsAvailable()) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Qt6 support is not available in this build")));
        return;
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(
        isolate,
        qt6::core::CleanPath(qt6::util::V8ValueToStdString(isolate, args[0])).c_str()).ToLocalChecked());
}

void ToNativeSeparatorsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "toNativeSeparators expects path string")));
        return;
    }

    if (!qt6::core::IsAvailable()) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Qt6 support is not available in this build")));
        return;
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(
        isolate,
        qt6::core::ToNativeSeparators(qt6::util::V8ValueToStdString(isolate, args[0])).c_str())
                                   .ToLocalChecked());
}

}  // namespace modules::qt_module_detail
