#include "modules/qt6/qt_gui_module.h"

#include "wrapper/qt6/effects/filter_v8.h"
#include "wrapper/qt6/core/path.h"
#include "wrapper/qt6/graphics/painter_v8.h"
#include "wrapper/qt6/gui/color_v8.h"
#include "wrapper/qt6/gui/font_v8.h"
#include "wrapper/qt6/gui/widget.h"
#include "wrapper/qt6/gui/widget_runtime.h"
#include "wrapper/qt6/image/image_v8.h"
#include "wrapper/qt6/util/v8_string.h"
#include "wrapper/qt6/v8/module_builder.h"

#if HAS_QT_WIDGETS
#include <QCoreApplication>
#include <QEventLoop>
#include <QScreen>
#endif

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#ifndef ENGINE_HAS_QT6_GUI
#define ENGINE_HAS_QT6_GUI 0
#endif

#ifndef ENGINE_HAS_QT6_WIDGETS
#define ENGINE_HAS_QT6_WIDGETS 0
#endif

namespace modules::qt_gui_module_detail {

void GuiIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_GUI == 1));
}

void WidgetsIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_WIDGETS == 1));
}

void GuiVersionCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(args.GetIsolate(), qt6::core::VersionString().c_str()).ToLocalChecked());
}

void AppIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_WIDGETS == 1));
}

void AppExecCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    qt6::gui::EnsureGuiApp();
    const int code = QApplication::exec();
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), code));
#else
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), -1));
#endif
}

void AppQuitCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    qt6::gui::EnsureGuiApp();
    QApplication::quit();
#else
    (void)args;
#endif
}

void AppProcessEventsCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    qt6::gui::EnsureGuiApp();
    int max_ms = 5;
    if (args.Length() >= 1 && args[0]->IsNumber()) {
        max_ms = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(5);
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, max_ms);
#else
    (void)args;
#endif
}

void AppScreenCountCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    qt6::gui::EnsureGuiApp();
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), QApplication::screens().size()));
#else
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
#endif
}

void AppPrimaryScreenWidthCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    qt6::gui::EnsureGuiApp();
    auto* screen = QApplication::primaryScreen();
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), screen ? screen->geometry().width() : 0));
#else
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
#endif
}

void AppPrimaryScreenHeightCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    qt6::gui::EnsureGuiApp();
    auto* screen = QApplication::primaryScreen();
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), screen ? screen->geometry().height() : 0));
#else
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
#endif
}

}  // namespace modules::qt_gui_module_detail

namespace modules {

bool RegisterQtGuiModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    using namespace qt6::v8bridge;

    v8::Local<v8::Object> gui_mod = v8::Object::New(isolate);
    bool ok = true;
    ok = ok && SetMethod(isolate, context, gui_mod, "isAvailable", qt_gui_module_detail::GuiIsAvailableCb);
    ok = ok && SetMethod(isolate, context, gui_mod, "version", qt_gui_module_detail::GuiVersionCb);
    if (!ok) return false;
    if (!ExportGlobalModule(isolate, context, "QtGui", gui_mod)) return false;

    v8::Local<v8::Object> widgets_mod = v8::Object::New(isolate);
    ok = true;
    ok = ok && SetMethod(isolate, context, widgets_mod, "isAvailable", qt_gui_module_detail::WidgetsIsAvailableCb);
    ok = ok && SetMethod(isolate, context, widgets_mod, "version", qt_gui_module_detail::GuiVersionCb);
    if (!ok) return false;
    if (!ExportGlobalModule(isolate, context, "QtWidgets", widgets_mod)) return false;

    v8::Local<v8::Object> app_mod = v8::Object::New(isolate);
    ok = true;
    ok = ok && SetMethod(isolate, context, app_mod, "isAvailable", qt_gui_module_detail::AppIsAvailableCb);
    ok = ok && SetMethod(isolate, context, app_mod, "exec", qt_gui_module_detail::AppExecCb);
    ok = ok && SetMethod(isolate, context, app_mod, "quit", qt_gui_module_detail::AppQuitCb);
    ok = ok && SetMethod(isolate, context, app_mod, "processEvents", qt_gui_module_detail::AppProcessEventsCb);
    ok = ok && SetMethod(isolate, context, app_mod, "screenCount", qt_gui_module_detail::AppScreenCountCb);
    ok = ok && SetMethod(isolate, context, app_mod, "primaryScreenWidth", qt_gui_module_detail::AppPrimaryScreenWidthCb);
    ok = ok && SetMethod(isolate, context, app_mod, "primaryScreenHeight", qt_gui_module_detail::AppPrimaryScreenHeightCb);
    if (!ok) return false;
    if (!ExportGlobalModule(isolate, context, "QtApplication", app_mod)) return false;

    // QtColor — colour utility functions
    if (!qt6::gui::RegisterQtColorModule(isolate, context))          return false;

    // QtFont — font utility functions
    if (!qt6::gui::RegisterQtFontModule(isolate, context))           return false;

    // QtWidget — windowed UI class
    if (!qt6::gui::RegisterQtWidgetClass(isolate, context))          return false;

    // QtPainterPath + QtDraw — vector drawing
    if (!qt6::graphics::RegisterQtPainterClass(isolate, context))    return false;

    // QtImage — raster image operations
    if (!qt6::image::RegisterQtImageModule(isolate, context))        return false;

    // QtImageFilter — image effect operations
    if (!qt6::effects::RegisterQtImageFilterModule(isolate, context)) return false;

    return true;
}

}  // namespace modules
