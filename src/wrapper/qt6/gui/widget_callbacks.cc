#include "wrapper/qt6/gui/widget.h"

#include "wrapper/qt6/v8/class_builder.h"
#include "wrapper/qt6/gui/widget_runtime.h"
#include "wrapper/qt6/ui/widget_factory.h"
#include "wrapper/qt6/ui/widget_ops.h"

#if HAS_QT_WIDGETS
#include <QBoxLayout>
#include <QEventLoop>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLayout>
#include <QScreen>
#include <QVBoxLayout>
#endif

#include <string>

namespace qt6::gui::widget_v8_detail {

using namespace qt6::v8bridge;

void ThrowUnavailable(v8::Isolate* isolate) {
    ThrowError(isolate, "Qt6 Widgets unavailable");
}

#if HAS_QT_WIDGETS
WidgetWrapper* GetSelf(const v8::FunctionCallbackInfo<v8::Value>& args) {
    return UnwrapPointer<WidgetWrapper>(args.This());
}

struct LabelCompatWrapper {
    QLabel* label = nullptr;
};

struct ControlCompatWrapper {
    QWidget* widget = nullptr;
};

struct LayoutCompatWrapper {
    QLayout* layout = nullptr;
};

bool IsInstanceOf(v8::Isolate* isolate,
                  v8::Local<v8::Object> obj,
                  const char* ctor_name) {
    auto ctx = isolate->GetCurrentContext();
    auto maybe_ctor = ctx->Global()->Get(ctx, ToV8Str(isolate, ctor_name));
    if (maybe_ctor.IsEmpty() || !maybe_ctor.ToLocalChecked()->IsFunction()) {
        return false;
    }
    auto ctor = maybe_ctor.ToLocalChecked().As<v8::Function>();
    return obj->InstanceOf(ctx, ctor).FromMaybe(false);
}

LabelCompatWrapper* GetLabelSelf(const v8::FunctionCallbackInfo<v8::Value>& args) {
    return UnwrapPointer<LabelCompatWrapper>(args.This());
}

ControlCompatWrapper* GetControlSelf(const v8::FunctionCallbackInfo<v8::Value>& args) {
    return UnwrapPointer<ControlCompatWrapper>(args.This());
}

LayoutCompatWrapper* GetLayoutSelf(const v8::FunctionCallbackInfo<v8::Value>& args) {
    return UnwrapPointer<LayoutCompatWrapper>(args.This());
}

QWidget* ExtractWidget(v8::Local<v8::Object> obj) {
    auto* isolate = obj->GetIsolate();

    if (IsInstanceOf(isolate, obj, "QtWidget")) {
        if (auto* ww = UnwrapPointer<WidgetWrapper>(obj)) return ww->get();
        return nullptr;
    }

    if (IsInstanceOf(isolate, obj, "QtWidgetLabel")) {
        if (auto* lw = UnwrapPointer<LabelCompatWrapper>(obj)) return lw->label;
        return nullptr;
    }

    static const char* kControlTypes[] = {
        "QtWidgetButton",
        "QtWidgetLineEdit",
        "QtWidgetTextEdit",
        "QtWidgetComboBox",
        "QtWidgetCheckBox",
        "QtWidgetRadioButton",
        "QtWidgetSpinBox",
        "QtWidgetDoubleSpinBox",
        "QtWidgetSlider",
        "QtWidgetDial",
        "QtWidgetProgressBar",
        "QtWidgetListWidget",
        "QtWidgetTableWidget",
        "QtWidgetTreeWidget",
        "QtWidgetDateEdit",
        "QtWidgetTimeEdit",
        "QtWidgetDateTimeEdit",
        "QtWidgetGroupBox",
        "QtWidgetTabWidget",
    };

    for (const char* type_name : kControlTypes) {
        if (IsInstanceOf(isolate, obj, type_name)) {
            if (auto* cw = UnwrapPointer<ControlCompatWrapper>(obj)) return cw->widget;
            return nullptr;
        }
    }

    return nullptr;
}

QWidget* ExtractParent(v8::Isolate* isolate, const v8::FunctionCallbackInfo<v8::Value>& args, int parent_index) {
    if (args.Length() <= parent_index || !args[parent_index]->IsObject()) {
        ThrowTypeError(isolate, "parentWidget must be an object");
        return nullptr;
    }
    auto* parent = ExtractWidget(args[parent_index].As<v8::Object>());
    if (!parent) {
        ThrowTypeError(isolate, "parentWidget must be QtWidget or QtWidget.*");
        return nullptr;
    }
    return parent;
}

void WrapCompatControl(v8::Isolate* isolate,
                       const v8::FunctionCallbackInfo<v8::Value>& args,
                       QWidget* widget) {
    auto* wrap = new ControlCompatWrapper{ widget };
    WrapPointer(args.This(), wrap);
    RegisterWeakCleanup(isolate, args.This(), wrap);
    args.GetReturnValue().Set(args.This());
}

void BuildControl(v8::Isolate* isolate,
                  const v8::FunctionCallbackInfo<v8::Value>& args,
                  std::unique_ptr<QWidget> control) {
    if (!control) {
        ThrowError(isolate, "Failed to create widget control");
        return;
    }
    QWidget* raw = control.release();
    WrapCompatControl(isolate, args, raw);
}
#endif

void WidgetCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget(title?)'");
        return;
    }

    std::string title;
    if (args.Length() >= 1 && args[0]->IsString()) {
        title = FromV8Str(args.GetIsolate(), args[0]);
    }

    auto* self = new WidgetWrapper(args.GetIsolate(),
                                   args.GetIsolate()->GetCurrentContext(),
                                   title);
    WrapPointer(args.This(), self);
    RegisterWeakCleanup(args.GetIsolate(), args.This(), self);
    args.GetReturnValue().Set(args.This());
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetSetTitle(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    self->get()->setWindowTitle(QString::fromUtf8(FromV8Str(args.GetIsolate(), args[0]).c_str()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetTitle(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), self->get()->windowTitle().toUtf8().constData()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetSetGeometry(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 4) return;
    auto ctx = args.GetIsolate()->GetCurrentContext();
    int x = args[0]->Int32Value(ctx).FromMaybe(0);
    int y = args[1]->Int32Value(ctx).FromMaybe(0);
    int w = args[2]->Int32Value(ctx).FromMaybe(640);
    int h = args[3]->Int32Value(ctx).FromMaybe(360);
    self->get()->setGeometry(x, y, w, h);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetResize(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 2) return;
    auto ctx = args.GetIsolate()->GetCurrentContext();
    int w = args[0]->Int32Value(ctx).FromMaybe(640);
    int h = args[1]->Int32Value(ctx).FromMaybe(360);
    self->get()->resize(w, h);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetMove(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 2) return;
    auto ctx = args.GetIsolate()->GetCurrentContext();
    int x = args[0]->Int32Value(ctx).FromMaybe(0);
    int y = args[1]->Int32Value(ctx).FromMaybe(0);
    self->get()->move(x, y);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetShow(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    self->get()->show();
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetShowMaximized(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    self->get()->showMaximized();
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetShowMinimized(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    self->get()->showMinimized();
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetShowFullScreen(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    self->get()->showFullScreen();
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetRaise(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    self->get()->raise();
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLower(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    self->get()->lower();
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetActivateWindow(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    self->get()->activateWindow();
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetHasFocus(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), self->get()->hasFocus()));
#else
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
#endif
}

void WidgetSetFocus(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    self->get()->setFocus();
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetSetWindowOpacity(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    double op = args[0]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(1.0);
    if (op < 0.0) op = 0.0;
    if (op > 1.0) op = 1.0;
    self->get()->setWindowOpacity(op);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetWindowOpacity(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), self->get()->windowOpacity()));
#else
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), 1.0));
#endif
}

void WidgetWindowState(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    const auto state = self->get()->windowState();
    std::string label = "normal";
    if (state.testFlag(Qt::WindowFullScreen)) label = "fullscreen";
    else if (state.testFlag(Qt::WindowMaximized)) label = "maximized";
    else if (state.testFlag(Qt::WindowMinimized)) label = "minimized";
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), label));
#else
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), "normal"));
#endif
}

void WidgetAddLabel(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 5) return;
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    int id = self->addLabel(
        FromV8Str(iso, args[0]),
        args[1]->Int32Value(ctx).FromMaybe(0),
        args[2]->Int32Value(ctx).FromMaybe(0),
        args[3]->Int32Value(ctx).FromMaybe(120),
        args[4]->Int32Value(ctx).FromMaybe(24));
    args.GetReturnValue().Set(v8::Integer::New(iso, id));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetAddButton(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 5) return;
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    int id = self->addButton(
        FromV8Str(iso, args[0]),
        args[1]->Int32Value(ctx).FromMaybe(0),
        args[2]->Int32Value(ctx).FromMaybe(0),
        args[3]->Int32Value(ctx).FromMaybe(120),
        args[4]->Int32Value(ctx).FromMaybe(28));
    args.GetReturnValue().Set(v8::Integer::New(iso, id));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetAddLineEdit(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 5) return;
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    int id = self->addLineEdit(
        FromV8Str(iso, args[0]),
        args[1]->Int32Value(ctx).FromMaybe(0),
        args[2]->Int32Value(ctx).FromMaybe(0),
        args[3]->Int32Value(ctx).FromMaybe(220),
        args[4]->Int32Value(ctx).FromMaybe(28));
    args.GetReturnValue().Set(v8::Integer::New(iso, id));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetAddTextEdit(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 4) return;
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    int id = self->addTextEdit(
        args[0]->Int32Value(ctx).FromMaybe(0),
        args[1]->Int32Value(ctx).FromMaybe(0),
        args[2]->Int32Value(ctx).FromMaybe(300),
        args[3]->Int32Value(ctx).FromMaybe(100));
    args.GetReturnValue().Set(v8::Integer::New(iso, id));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetAddComboBox(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 4) return;
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    int id = self->addComboBox(
        args[0]->Int32Value(ctx).FromMaybe(0),
        args[1]->Int32Value(ctx).FromMaybe(0),
        args[2]->Int32Value(ctx).FromMaybe(150),
        args[3]->Int32Value(ctx).FromMaybe(28));
    args.GetReturnValue().Set(v8::Integer::New(iso, id));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetAddCheckBox(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 5) return;
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    int id = self->addCheckBox(
        FromV8Str(iso, args[0]),
        args[1]->Int32Value(ctx).FromMaybe(0),
        args[2]->Int32Value(ctx).FromMaybe(0),
        args[3]->Int32Value(ctx).FromMaybe(100),
        args[4]->Int32Value(ctx).FromMaybe(24));
    args.GetReturnValue().Set(v8::Integer::New(iso, id));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetAddRadioButton(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 5) return;
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    int id = self->addRadioButton(
        FromV8Str(iso, args[0]),
        args[1]->Int32Value(ctx).FromMaybe(0),
        args[2]->Int32Value(ctx).FromMaybe(0),
        args[3]->Int32Value(ctx).FromMaybe(100),
        args[4]->Int32Value(ctx).FromMaybe(24));
    args.GetReturnValue().Set(v8::Integer::New(iso, id));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetAddSpinBox(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 7) return;
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    int id = self->addSpinBox(
        args[0]->Int32Value(ctx).FromMaybe(0),
        args[1]->Int32Value(ctx).FromMaybe(0),
        args[2]->Int32Value(ctx).FromMaybe(100),
        args[3]->Int32Value(ctx).FromMaybe(0),
        args[4]->Int32Value(ctx).FromMaybe(0),
        args[5]->Int32Value(ctx).FromMaybe(80),
        args[6]->Int32Value(ctx).FromMaybe(28));
    args.GetReturnValue().Set(v8::Integer::New(iso, id));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetAddSlider(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 8) return;
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    int id = self->addSlider(
        args[0]->Int32Value(ctx).FromMaybe(0),
        args[1]->Int32Value(ctx).FromMaybe(0),
        args[2]->Int32Value(ctx).FromMaybe(100),
        args[3]->Int32Value(ctx).FromMaybe(50),
        args[4]->Int32Value(ctx).FromMaybe(0),
        args[5]->Int32Value(ctx).FromMaybe(0),
        args[6]->Int32Value(ctx).FromMaybe(200),
        args[7]->Int32Value(ctx).FromMaybe(28));
    args.GetReturnValue().Set(v8::Integer::New(iso, id));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetAddProgressBar(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 7) return;
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    int id = self->addProgressBar(
        args[0]->Int32Value(ctx).FromMaybe(0),
        args[1]->Int32Value(ctx).FromMaybe(100),
        args[2]->Int32Value(ctx).FromMaybe(0),
        args[3]->Int32Value(ctx).FromMaybe(0),
        args[4]->Int32Value(ctx).FromMaybe(0),
        args[5]->Int32Value(ctx).FromMaybe(220),
        args[6]->Int32Value(ctx).FromMaybe(24));
    args.GetReturnValue().Set(v8::Integer::New(iso, id));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetAddListWidget(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 4) return;
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    int id = self->addListWidget(
        args[0]->Int32Value(ctx).FromMaybe(0),
        args[1]->Int32Value(ctx).FromMaybe(0),
        args[2]->Int32Value(ctx).FromMaybe(220),
        args[3]->Int32Value(ctx).FromMaybe(120));
    args.GetReturnValue().Set(v8::Integer::New(iso, id));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetControlExists(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), self->hasControl(id)));
#else
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
#endif
}

void WidgetControlIds(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    auto ids = self->controlIds();
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    auto arr = v8::Array::New(iso, static_cast<int>(ids.size()));
    for (size_t i = 0; i < ids.size(); ++i) {
        arr->Set(ctx, static_cast<uint32_t>(i), v8::Integer::New(iso, ids[i])).Check();
    }
    args.GetReturnValue().Set(arr);
#else
    args.GetReturnValue().Set(v8::Array::New(args.GetIsolate(), 0));
#endif
}

void WidgetRemoveControl(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), self->removeControl(id)));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetClearControls(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    self->clearControls();
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetSetControlText(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 2) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    auto ok = self->setControlText(id, FromV8Str(args.GetIsolate(), args[1]));
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetControlText(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), self->controlText(id)));
#else
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), ""));
#endif
}

void WidgetSetControlValue(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 2 || !args[1]->IsNumber()) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    double value = args[1]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0.0);
    auto ok = self->setControlValue(id, value);
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetControlValue(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    bool ok = false;
    const double value = self->controlValue(id, &ok);
    if (!ok) {
        args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
        return;
    }
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), value));
#else
    args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
#endif
}

void WidgetSetControlChecked(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 2) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    bool checked = args[1]->BooleanValue(args.GetIsolate());
    auto ok = self->setControlChecked(id, checked);
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetControlChecked(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    bool ok = false;
    const bool checked = self->controlChecked(id, &ok);
    if (!ok) {
        args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
        return;
    }
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), checked));
#else
    args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
#endif
}

void WidgetAddComboItem(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 2) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    auto ok = self->addComboItem(id, FromV8Str(args.GetIsolate(), args[1]));
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetClearComboItems(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    auto ok = self->clearComboItems(id);
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetAddListItem(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 2) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    auto ok = self->addListItem(id, FromV8Str(args.GetIsolate(), args[1]));
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetClearListItems(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    auto ok = self->clearListItems(id);
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetSetPlaceholder(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 2) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    auto ok = self->setPlaceholder(id, FromV8Str(args.GetIsolate(), args[1]));
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetSetControlGeometry(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 5) return;
    auto ctx = args.GetIsolate()->GetCurrentContext();
    int id = args[0]->Int32Value(ctx).FromMaybe(0);
    int x = args[1]->Int32Value(ctx).FromMaybe(0);
    int y = args[2]->Int32Value(ctx).FromMaybe(0);
    int w = args[3]->Int32Value(ctx).FromMaybe(100);
    int h = args[4]->Int32Value(ctx).FromMaybe(24);
    bool ok = self->setControlGeometry(id, x, y, w, h);
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetOnClicked(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 2 || !args[1]->IsFunction()) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    bool ok = self->setOnClicked(id, args[1].As<v8::Function>());
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetOnTextChanged(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 2 || !args[1]->IsFunction()) return;
    int id = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    bool ok = self->setOnTextChanged(id, args[1].As<v8::Function>());
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetHide(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    self->get()->hide();
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetClose(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    self->get()->close();
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetIsVisible(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), self->get()->isVisible()));
#else
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
#endif
}

void WidgetSetEnabled(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    self->get()->setEnabled(args[0]->BooleanValue(args.GetIsolate()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetIsEnabled(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), self->get()->isEnabled()));
#else
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
#endif
}

void WidgetSetStyleSheet(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    self->get()->setStyleSheet(QString::fromUtf8(FromV8Str(args.GetIsolate(), args[0]).c_str()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetSetToolTip(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    self->get()->setToolTip(QString::fromUtf8(FromV8Str(args.GetIsolate(), args[0]).c_str()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetSetMinimumSize(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 2) return;
    auto ctx = args.GetIsolate()->GetCurrentContext();
    self->get()->setMinimumSize(
        args[0]->Int32Value(ctx).FromMaybe(0),
        args[1]->Int32Value(ctx).FromMaybe(0));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetSetMaximumSize(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 2) return;
    auto ctx = args.GetIsolate()->GetCurrentContext();
    self->get()->setMaximumSize(
        args[0]->Int32Value(ctx).FromMaybe(16777215),
        args[1]->Int32Value(ctx).FromMaybe(16777215));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetSetFixedSize(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self || args.Length() < 2) return;
    auto ctx = args.GetIsolate()->GetCurrentContext();
    self->get()->setFixedSize(
        args[0]->Int32Value(ctx).FromMaybe(640),
        args[1]->Int32Value(ctx).FromMaybe(360));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetX(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), self->get()->x()));
#else
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
#endif
}

void WidgetY(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), self->get()->y()));
#else
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
#endif
}

void WidgetWidth(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), self->get()->width()));
#else
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
#endif
}

void WidgetHeight(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), self->get()->height()));
#else
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
#endif
}

void WidgetSize(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    auto out = v8::Object::New(iso);
    out->Set(ctx, ToV8Str(iso, "width"), v8::Integer::New(iso, self->get()->width())).Check();
    out->Set(ctx, ToV8Str(iso, "height"), v8::Integer::New(iso, self->get()->height())).Check();
    args.GetReturnValue().Set(out);
#else
    args.GetReturnValue().Set(v8::Object::New(args.GetIsolate()));
#endif
}

void WidgetIsLayout(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), self->get()->layout() != nullptr));
#else
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
#endif
}

void WidgetCreateLayout(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    auto* root = self->get();
    if (root->layout() != nullptr) {
        args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
        return;
    }
    int type = 1;
    if (args.Length() >= 1 && args[0]->IsNumber()) {
        type = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(1);
    }
    if (type == 1) {
        auto* layout = new QVBoxLayout(root);
        root->setLayout(layout);
        args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
        return;
    }
    if (type == 2) {
        auto* layout = new QHBoxLayout(root);
        root->setLayout(layout);
        args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
        return;
    }
    if (type == 3) {
        auto* layout = new QGridLayout(root);
        root->setLayout(layout);
        args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
        return;
    }
    if (type == 4) {
        auto* layout = new QFormLayout(root);
        root->setLayout(layout);
        args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
        return;
    }
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
#else
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
#endif
}

void WidgetGetLayout(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetSelf(args);
    if (!self) return;
    auto* layout = self->get()->layout();
    if (!layout) {
        args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
        return;
    }
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    auto maybe_ctor = ctx->Global()->Get(ctx, ToV8Str(iso, "QtWidgetLayout"));
    if (maybe_ctor.IsEmpty() || !maybe_ctor.ToLocalChecked()->IsFunction()) {
        args.GetReturnValue().Set(v8::Null(iso));
        return;
    }
    auto ctor = maybe_ctor.ToLocalChecked().As<v8::Function>();
    auto maybe_obj = ctor->NewInstance(ctx, 0, nullptr);
    if (maybe_obj.IsEmpty()) {
        args.GetReturnValue().Set(v8::Null(iso));
        return;
    }
    auto* wrap = new LayoutCompatWrapper{ layout };
    auto obj = maybe_obj.ToLocalChecked();
    WrapPointer(obj, wrap);
    RegisterWeakCleanup(iso, obj, wrap);
    args.GetReturnValue().Set(obj);
#else
    args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
#endif
}

void WidgetStaticIsAvailable(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), HAS_QT_WIDGETS == 1));
}

void WidgetStaticProcessEvents(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    EnsureGuiApp();
    int max_ms = 5;
    if (args.Length() >= 1 && args[0]->IsNumber()) {
        max_ms = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(5);
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, max_ms);
#else
    (void)args;
#endif
}

void WidgetStaticExec(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    EnsureGuiApp();
    int code = QApplication::exec();
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), code));
#else
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), -1));
#endif
}

void WidgetStaticQuit(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    EnsureGuiApp();
    QApplication::quit();
#else
    (void)args;
#endif
}

void WidgetStaticScreenCount(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    EnsureGuiApp();
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), QApplication::screens().size()));
#else
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
#endif
}

void WidgetStaticPrimaryScreenWidth(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    EnsureGuiApp();
    auto* screen = QApplication::primaryScreen();
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), screen ? screen->geometry().width() : 0));
#else
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
#endif
}

void WidgetStaticPrimaryScreenHeight(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    EnsureGuiApp();
    auto* screen = QApplication::primaryScreen();
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), screen ? screen->geometry().height() : 0));
#else
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
#endif
}

void WidgetLabelCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.Label(text, parentWidget)'");
        return;
    }
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsObject()) {
        ThrowTypeError(args.GetIsolate(), "QtWidget.Label(text, parentWidget) expected");
        return;
    }

    auto* parent = ExtractWidget(args[1].As<v8::Object>());
    if (!parent) {
        ThrowTypeError(args.GetIsolate(), "parentWidget must be QtWidget or QtWidget.Label");
        return;
    }
    auto text = FromV8Str(args.GetIsolate(), args[0]);
    auto control = qt6::ui::MakeLabel(parent, text, 0, 0, 120, 24);
    auto* label = qobject_cast<QLabel*>(control.get());
    if (!label) {
        ThrowError(args.GetIsolate(), "Failed to create QtWidget.Label");
        return;
    }
    (void)control.release();
    auto* wrap = new LabelCompatWrapper{ label };
    WrapPointer(args.This(), wrap);
    RegisterWeakCleanup(args.GetIsolate(), args.This(), wrap);
    args.GetReturnValue().Set(args.This());
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLabelSetAlignment(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetLabelSelf(args);
    if (!self || !self->label || args.Length() < 1 || !args[0]->IsNumber()) return;
    const int align = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    self->label->setAlignment(static_cast<Qt::Alignment>(align));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLabelResize(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetLabelSelf(args);
    if (!self || !self->label || args.Length() < 1) return;
    auto ctx = args.GetIsolate()->GetCurrentContext();
    int w = 0;
    int h = 0;
    if (args[0]->IsObject()) {
        auto obj = args[0].As<v8::Object>();
        auto width_v = obj->Get(ctx, ToV8Str(args.GetIsolate(), "width"));
        auto height_v = obj->Get(ctx, ToV8Str(args.GetIsolate(), "height"));
        if (width_v.IsEmpty() || height_v.IsEmpty()) return;
        w = width_v.ToLocalChecked()->Int32Value(ctx).FromMaybe(0);
        h = height_v.ToLocalChecked()->Int32Value(ctx).FromMaybe(0);
    } else if (args.Length() >= 2) {
        w = args[0]->Int32Value(ctx).FromMaybe(0);
        h = args[1]->Int32Value(ctx).FromMaybe(0);
    } else {
        return;
    }
    self->label->resize(w, h);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLabelSetParent(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetLabelSelf(args);
    if (!self || !self->label || args.Length() < 1 || !args[0]->IsObject()) return;
    auto* parent = ExtractWidget(args[0].As<v8::Object>());
    if (!parent) return;
    self->label->setParent(parent);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLabelSetPosition(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetLabelSelf(args);
    if (!self || !self->label || args.Length() < 2) return;
    auto ctx = args.GetIsolate()->GetCurrentContext();
    self->label->move(
        args[0]->Int32Value(ctx).FromMaybe(0),
        args[1]->Int32Value(ctx).FromMaybe(0));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLabelSetStyle(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetLabelSelf(args);
    if (!self || !self->label || args.Length() < 1) return;
    self->label->setStyleSheet(QString::fromUtf8(FromV8Str(args.GetIsolate(), args[0]).c_str()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLabelSetText(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetLabelSelf(args);
    if (!self || !self->label || args.Length() < 1) return;
    self->label->setText(QString::fromUtf8(FromV8Str(args.GetIsolate(), args[0]).c_str()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLabelText(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetLabelSelf(args);
    if (!self || !self->label) return;
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), self->label->text().toUtf8().constData()));
#else
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), ""));
#endif
}

void WidgetButtonCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.Button(text, parentWidget)'");
        return;
    }
    if (args.Length() < 2 || !args[0]->IsString()) {
        ThrowTypeError(args.GetIsolate(), "QtWidget.Button(text, parentWidget) expected");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 1);
    if (!parent) return;
    auto text = FromV8Str(args.GetIsolate(), args[0]);
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeButton(parent, text, 0, 0, 120, 28));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLineEditCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.LineEdit(placeholder, parentWidget)'");
        return;
    }
    if (args.Length() < 2 || !args[0]->IsString()) {
        ThrowTypeError(args.GetIsolate(), "QtWidget.LineEdit(placeholder, parentWidget) expected");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 1);
    if (!parent) return;
    auto placeholder = FromV8Str(args.GetIsolate(), args[0]);
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeLineEdit(parent, placeholder, 0, 0, 180, 28));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetTextEditCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.TextEdit(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeTextEdit(parent, 0, 0, 240, 120));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetComboBoxCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.ComboBox(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeComboBox(parent, 0, 0, 160, 28));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetCheckBoxCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.CheckBox(text, parentWidget)'");
        return;
    }
    if (args.Length() < 2 || !args[0]->IsString()) {
        ThrowTypeError(args.GetIsolate(), "QtWidget.CheckBox(text, parentWidget) expected");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 1);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeCheckBox(parent, FromV8Str(args.GetIsolate(), args[0]), 0, 0, 160, 24));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetRadioButtonCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.RadioButton(text, parentWidget)'");
        return;
    }
    if (args.Length() < 2 || !args[0]->IsString()) {
        ThrowTypeError(args.GetIsolate(), "QtWidget.RadioButton(text, parentWidget) expected");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 1);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeRadioButton(parent, FromV8Str(args.GetIsolate(), args[0]), 0, 0, 160, 24));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetSpinBoxCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.SpinBox(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeSpinBox(parent, 0, 0, 100, 0, 0, 100, 28));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetDoubleSpinBoxCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.DoubleSpinBox(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeDoubleSpinBox(parent, 0.0, 0.0, 100.0, 0, 0, 120, 28));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetSliderCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.Slider(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeSlider(parent, 0, 0, 100, 0, 0, 0, 200, 24));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetDialCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.Dial(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeDial(parent, 0, 100, 0, 0, 0, 80, 80));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetProgressBarCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.ProgressBar(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeProgressBar(parent, 0, 100, 0, 0, 0, 220, 24));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetListWidgetCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.ListWidget(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeListWidget(parent, 0, 0, 220, 120));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetTableWidgetCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.TableWidget(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeTableWidget(parent, 0, 0, 0, 0, 240, 140));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetTreeWidgetCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.TreeWidget(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeTreeWidget(parent, 0, 0, 240, 140));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetDateEditCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.DateEdit(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeDateEdit(parent, 0, 0, 140, 28));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetTimeEditCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.TimeEdit(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeTimeEdit(parent, 0, 0, 140, 28));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetDateTimeEditCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.DateTimeEdit(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeDateTimeEdit(parent, 0, 0, 170, 28));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetGroupBoxCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.GroupBox(title, parentWidget)'");
        return;
    }
    if (args.Length() < 2 || !args[0]->IsString()) {
        ThrowTypeError(args.GetIsolate(), "QtWidget.GroupBox(title, parentWidget) expected");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 1);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeGroupBox(parent, FromV8Str(args.GetIsolate(), args[0]), 0, 0, 240, 140));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetTabWidgetCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWidget.TabWidget(parentWidget)'");
        return;
    }
    auto* parent = ExtractParent(args.GetIsolate(), args, 0);
    if (!parent) return;
    BuildControl(args.GetIsolate(), args, qt6::ui::MakeTabWidget(parent, 0, 0, 260, 180));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetCompatControlSetParent(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget || args.Length() < 1 || !args[0]->IsObject()) return;
    auto* parent = ExtractWidget(args[0].As<v8::Object>());
    if (!parent) return;
    self->widget->setParent(parent);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetCompatControlSetPosition(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget || args.Length() < 2) return;
    auto ctx = args.GetIsolate()->GetCurrentContext();
    self->widget->move(args[0]->Int32Value(ctx).FromMaybe(0), args[1]->Int32Value(ctx).FromMaybe(0));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetCompatControlResize(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget || args.Length() < 1) return;
    auto ctx = args.GetIsolate()->GetCurrentContext();
    int w = 0;
    int h = 0;
    if (args[0]->IsObject()) {
        auto obj = args[0].As<v8::Object>();
        auto width_v = obj->Get(ctx, ToV8Str(args.GetIsolate(), "width"));
        auto height_v = obj->Get(ctx, ToV8Str(args.GetIsolate(), "height"));
        if (width_v.IsEmpty() || height_v.IsEmpty()) return;
        w = width_v.ToLocalChecked()->Int32Value(ctx).FromMaybe(0);
        h = height_v.ToLocalChecked()->Int32Value(ctx).FromMaybe(0);
    } else if (args.Length() >= 2) {
        w = args[0]->Int32Value(ctx).FromMaybe(0);
        h = args[1]->Int32Value(ctx).FromMaybe(0);
    } else {
        return;
    }
    self->widget->resize(w, h);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetCompatControlSetStyle(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget || args.Length() < 1) return;
    self->widget->setStyleSheet(QString::fromUtf8(FromV8Str(args.GetIsolate(), args[0]).c_str()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetCompatControlSetText(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget || args.Length() < 1) return;
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), qt6::ui::SetWidgetText(self->widget, FromV8Str(args.GetIsolate(), args[0]))));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetCompatControlText(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget) return;
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), qt6::ui::WidgetText(self->widget)));
#else
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), ""));
#endif
}

void WidgetCompatControlSetValue(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget || args.Length() < 1 || !args[0]->IsNumber()) return;
    const double value = args[0]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(0.0);
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), qt6::ui::SetWidgetValue(self->widget, value)));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetCompatControlValue(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget) return;
    double out = 0.0;
    if (!qt6::ui::WidgetValue(self->widget, &out)) {
        args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
        return;
    }
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), out));
#else
    args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
#endif
}

void WidgetCompatControlSetChecked(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget || args.Length() < 1) return;
    const bool checked = args[0]->BooleanValue(args.GetIsolate());
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), qt6::ui::SetWidgetChecked(self->widget, checked)));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetCompatControlChecked(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget) return;
    bool out = false;
    if (!qt6::ui::WidgetChecked(self->widget, &out)) {
        args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
        return;
    }
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), out));
#else
    args.GetReturnValue().Set(v8::Null(args.GetIsolate()));
#endif
}

void WidgetCompatControlAddItem(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget || args.Length() < 1) return;
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), qt6::ui::AddWidgetItem(self->widget, FromV8Str(args.GetIsolate(), args[0]))));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetCompatControlClearItems(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget) return;
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), qt6::ui::ClearWidgetItems(self->widget)));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetCompatControlSetPlaceholder(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget || args.Length() < 1) return;
    auto* line = qobject_cast<QLineEdit*>(self->widget);
    if (!line) {
        args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
        return;
    }
    line->setPlaceholderText(QString::fromUtf8(FromV8Str(args.GetIsolate(), args[0]).c_str()));
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetCompatControlSetAlignment(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetControlSelf(args);
    if (!self || !self->widget || args.Length() < 1 || !args[0]->IsNumber()) return;
    auto* label = qobject_cast<QLabel*>(self->widget);
    if (!label) {
        args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
        return;
    }
    const int align = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    label->setAlignment(static_cast<Qt::Alignment>(align));
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLayoutCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "QtWidgetLayout is internal");
        return;
    }
    auto* wrap = new LayoutCompatWrapper{};
    WrapPointer(args.This(), wrap);
    RegisterWeakCleanup(args.GetIsolate(), args.This(), wrap);
    args.GetReturnValue().Set(args.This());
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLayoutAddWidget(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetLayoutSelf(args);
    if (!self || !self->layout || args.Length() < 1 || !args[0]->IsObject()) return;
    auto* w = ExtractWidget(args[0].As<v8::Object>());
    if (!w) return;
    if (auto* grid = qobject_cast<QGridLayout*>(self->layout)) {
        auto ctx = args.GetIsolate()->GetCurrentContext();
        int row = args.Length() >= 2 ? args[1]->Int32Value(ctx).FromMaybe(0) : 0;
        int col = args.Length() >= 3 ? args[2]->Int32Value(ctx).FromMaybe(0) : 0;
        int row_span = args.Length() >= 4 ? args[3]->Int32Value(ctx).FromMaybe(1) : 1;
        int col_span = args.Length() >= 5 ? args[4]->Int32Value(ctx).FromMaybe(1) : 1;
        grid->addWidget(w, row, col, row_span, col_span);
        return;
    }
    self->layout->addWidget(w);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLayoutAddRow(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetLayoutSelf(args);
    if (!self || !self->layout) return;
    auto* form = qobject_cast<QFormLayout*>(self->layout);
    if (!form || args.Length() < 2 || !args[0]->IsObject() || !args[1]->IsObject()) return;
    auto* left = ExtractWidget(args[0].As<v8::Object>());
    auto* right = ExtractWidget(args[1].As<v8::Object>());
    if (!left || !right) return;
    form->addRow(left, right);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLayoutAddSpacing(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetLayoutSelf(args);
    if (!self || !self->layout || args.Length() < 1) return;
    const int size = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    if (auto* box = qobject_cast<QBoxLayout*>(self->layout)) {
        box->addSpacing(size);
        return;
    }
    self->layout->setSpacing(size);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void WidgetLayoutAddStretch(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WIDGETS
    auto* self = GetLayoutSelf(args);
    if (!self || !self->layout) return;
    int stretch = 1;
    if (args.Length() >= 1 && args[0]->IsNumber()) {
        stretch = args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(1);
    }
    if (auto* box = qobject_cast<QBoxLayout*>(self->layout)) {
        box->addStretch(stretch);
    }
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

}  // namespace qt6::gui::widget_v8_detail
