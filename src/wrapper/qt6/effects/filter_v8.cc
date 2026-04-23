#include "wrapper/qt6/effects/filter_v8.h"
#include "wrapper/qt6/effects/filter.h"
#include "wrapper/qt6/util/v8_string.h"
#include "wrapper/qt6/v8/class_builder.h"
#include "wrapper/qt6/v8/module_builder.h"

namespace {

using namespace qt6::v8bridge;

void FilterBlurCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 2) {
        ThrowTypeError(iso, "QtImageFilter.blur(src, dst, radius?, gaussian?)");
        return;
    }
    auto src = qt6::util::V8ValueToStdString(iso, args[0]);
    auto dst = qt6::util::V8ValueToStdString(iso, args[1]);
    qt6::effects::BlurOptions opts;
    if (args.Length() >= 3 && args[2]->IsNumber())
        opts.radius = args[2]->NumberValue(ctx).FromMaybe(5.0);
    if (args.Length() >= 4)
        opts.gaussian = args[3]->BooleanValue(iso);
    args.GetReturnValue().Set(v8::Boolean::New(iso, qt6::effects::ApplyBlur(src, dst, opts)));
}

void FilterDropShadowCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 2) {
        ThrowTypeError(iso, "QtImageFilter.dropShadow(src, dst, blurRadius?, offsetX?, offsetY?, color?)");
        return;
    }
    auto src = qt6::util::V8ValueToStdString(iso, args[0]);
    auto dst = qt6::util::V8ValueToStdString(iso, args[1]);
    qt6::effects::DropShadowOptions opts;
    if (args.Length() >= 3 && args[2]->IsNumber())
        opts.blur_radius = args[2]->NumberValue(ctx).FromMaybe(5.0);
    if (args.Length() >= 4 && args[3]->IsNumber())
        opts.offset_x = args[3]->NumberValue(ctx).FromMaybe(2.0);
    if (args.Length() >= 5 && args[4]->IsNumber())
        opts.offset_y = args[4]->NumberValue(ctx).FromMaybe(2.0);
    if (args.Length() >= 6 && args[5]->IsString())
        opts.color = qt6::util::V8ValueToStdString(iso, args[5]);
    args.GetReturnValue().Set(
        v8::Boolean::New(iso, qt6::effects::ApplyDropShadow(src, dst, opts)));
}

void FilterGlowCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 2) {
        ThrowTypeError(iso, "QtImageFilter.glow(src, dst, radius?, color?)");
        return;
    }
    auto src = qt6::util::V8ValueToStdString(iso, args[0]);
    auto dst = qt6::util::V8ValueToStdString(iso, args[1]);
    qt6::effects::GlowOptions opts;
    if (args.Length() >= 3 && args[2]->IsNumber())
        opts.radius = args[2]->NumberValue(ctx).FromMaybe(8.0);
    if (args.Length() >= 4 && args[3]->IsString())
        opts.color = qt6::util::V8ValueToStdString(iso, args[3]);
    args.GetReturnValue().Set(v8::Boolean::New(iso, qt6::effects::ApplyGlow(src, dst, opts)));
}

void FilterBrightnessContrastCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 2) {
        ThrowTypeError(iso, "QtImageFilter.adjustBrightnessContrast(src, dst, brightness?, contrast?)");
        return;
    }
    auto src = qt6::util::V8ValueToStdString(iso, args[0]);
    auto dst = qt6::util::V8ValueToStdString(iso, args[1]);
    int  b   = args.Length() >= 3 ? args[2]->Int32Value(ctx).FromMaybe(100) : 100;
    int  c   = args.Length() >= 4 ? args[3]->Int32Value(ctx).FromMaybe(100) : 100;
    args.GetReturnValue().Set(
        v8::Boolean::New(iso, qt6::effects::AdjustBrightnessContrast(src, dst, b, c)));
}

void FilterTintCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    if (args.Length() < 3) {
        ThrowTypeError(iso, "QtImageFilter.tint(src, dst, color)");
        return;
    }
    auto src   = qt6::util::V8ValueToStdString(iso, args[0]);
    auto dst   = qt6::util::V8ValueToStdString(iso, args[1]);
    auto color = qt6::util::V8ValueToStdString(iso, args[2]);
    args.GetReturnValue().Set(v8::Boolean::New(iso, qt6::effects::ApplyTint(src, dst, color)));
}

void FilterIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if ENGINE_HAS_QT6
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
#else
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
#endif
}

}  // namespace

namespace qt6::effects {

bool RegisterQtImageFilterModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope scope(isolate);
    using namespace qt6::v8bridge;

    auto mod = v8::Object::New(isolate);
    bool ok = true;
    ok = ok && SetMethod(isolate, context, mod, "isAvailable",            FilterIsAvailableCb);
    ok = ok && SetMethod(isolate, context, mod, "blur",                   FilterBlurCb);
    ok = ok && SetMethod(isolate, context, mod, "dropShadow",             FilterDropShadowCb);
    ok = ok && SetMethod(isolate, context, mod, "glow",                   FilterGlowCb);
    ok = ok && SetMethod(isolate, context, mod, "adjustBrightnessContrast", FilterBrightnessContrastCb);
    ok = ok && SetMethod(isolate, context, mod, "tint",                   FilterTintCb);
    if (!ok) return false;
    return ExportGlobalModule(isolate, context, "QtImageFilter", mod);
}

}  // namespace qt6::effects
