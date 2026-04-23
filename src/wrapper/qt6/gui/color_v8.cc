#include "wrapper/qt6/gui/color_v8.h"
#include "wrapper/qt6/gui/color.h"
#include "wrapper/qt6/util/v8_string.h"
#include "wrapper/qt6/v8/class_builder.h"
#include "wrapper/qt6/v8/module_builder.h"

namespace {

using namespace qt6::v8bridge;

void ThrowLocalTypeError(v8::Isolate* iso, const char* message) {
    iso->ThrowException(v8::Exception::TypeError(
        v8::String::NewFromUtf8(iso, message).ToLocalChecked()));
}

void ColorParseCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) {
        ThrowLocalTypeError(iso, "QtColor.parse(css: string)");
        return;
    }
    auto c = qt6::gui::ParseColor(qt6::util::V8ValueToStdString(iso, args[0]));
    auto obj = v8::Object::New(iso);
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "r"), v8::Integer::New(iso, c.r)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "g"), v8::Integer::New(iso, c.g)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "b"), v8::Integer::New(iso, c.b)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "a"), v8::Integer::New(iso, c.a)).Check();
    args.GetReturnValue().Set(obj);
}

void ColorToHexCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    qt6::gui::ColorRgba c;
    if (args.Length() >= 1 && args[0]->IsNumber())
        c.r = args[0]->Int32Value(ctx).FromMaybe(0);
    if (args.Length() >= 2 && args[1]->IsNumber())
        c.g = args[1]->Int32Value(ctx).FromMaybe(0);
    if (args.Length() >= 3 && args[2]->IsNumber())
        c.b = args[2]->Int32Value(ctx).FromMaybe(0);
    if (args.Length() >= 4 && args[3]->IsNumber())
        c.a = args[3]->Int32Value(ctx).FromMaybe(255);
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, qt6::gui::ColorToHex(c).c_str()).ToLocalChecked());
}

void ColorToRgbaCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    qt6::gui::ColorRgba c;
    if (args.Length() >= 1 && args[0]->IsNumber())
        c.r = args[0]->Int32Value(ctx).FromMaybe(0);
    if (args.Length() >= 2 && args[1]->IsNumber())
        c.g = args[1]->Int32Value(ctx).FromMaybe(0);
    if (args.Length() >= 3 && args[2]->IsNumber())
        c.b = args[2]->Int32Value(ctx).FromMaybe(0);
    if (args.Length() >= 4 && args[3]->IsNumber())
        c.a = args[3]->Int32Value(ctx).FromMaybe(255);
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, qt6::gui::ColorToRgbaString(c).c_str()).ToLocalChecked());
}

void ColorIsValidCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    if (args.Length() < 1) {
        args.GetReturnValue().Set(v8::Boolean::New(iso, false));
        return;
    }
    bool valid = qt6::gui::IsValidColor(qt6::util::V8ValueToStdString(iso, args[0]));
    args.GetReturnValue().Set(v8::Boolean::New(iso, valid));
}

// Mix two colours by fraction (0=first, 1=second)
void ColorMixCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    // mix(css1, css2, t=0.5) -> hex
    if (args.Length() < 2) {
        ThrowLocalTypeError(iso, "QtColor.mix(css1, css2, t?)");
        return;
    }
    auto c1 = qt6::gui::ParseColor(qt6::util::V8ValueToStdString(iso, args[0]));
    auto c2 = qt6::gui::ParseColor(qt6::util::V8ValueToStdString(iso, args[1]));
    double t = 0.5;
    if (args.Length() >= 3 && args[2]->IsNumber())
        t = args[2]->NumberValue(ctx).FromMaybe(0.5);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    qt6::gui::ColorRgba out;
    out.r = static_cast<int>(c1.r + (c2.r - c1.r) * t);
    out.g = static_cast<int>(c1.g + (c2.g - c1.g) * t);
    out.b = static_cast<int>(c1.b + (c2.b - c1.b) * t);
    out.a = static_cast<int>(c1.a + (c2.a - c1.a) * t);
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, qt6::gui::ColorToHex(out).c_str()).ToLocalChecked());
}

// Lighten/darken by factor (factor > 1 = lighter, < 1 = darker)
void ColorScaleBrightnessCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 2) {
        ThrowLocalTypeError(iso, "QtColor.scaleBrightness(css, factor)");
        return;
    }
    auto c = qt6::gui::ParseColor(qt6::util::V8ValueToStdString(iso, args[0]));
    double f = args[1]->NumberValue(ctx).FromMaybe(1.0);
    auto clamp = [](int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); };
    c.r = clamp(static_cast<int>(c.r * f));
    c.g = clamp(static_cast<int>(c.g * f));
    c.b = clamp(static_cast<int>(c.b * f));
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, qt6::gui::ColorToHex(c).c_str()).ToLocalChecked());
}

// withAlpha(css, alpha: 0-255) -> hex
void ColorWithAlphaCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 2) {
        ThrowLocalTypeError(iso, "QtColor.withAlpha(css, alpha)");
        return;
    }
    auto c = qt6::gui::ParseColor(qt6::util::V8ValueToStdString(iso, args[0]));
    c.a = args[1]->Int32Value(ctx).FromMaybe(255);
    if (c.a < 0) c.a = 0;
    if (c.a > 255) c.a = 255;
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, qt6::gui::ColorToHex(c).c_str()).ToLocalChecked());
}

}  // namespace

namespace qt6::gui {

bool RegisterQtColorModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope scope(isolate);
    using namespace qt6::v8bridge;

    auto mod = v8::Object::New(isolate);
    bool ok = true;
    ok = ok && SetMethod(isolate, context, mod, "parse",            ColorParseCb);
    ok = ok && SetMethod(isolate, context, mod, "toHex",            ColorToHexCb);
    ok = ok && SetMethod(isolate, context, mod, "toRgba",           ColorToRgbaCb);
    ok = ok && SetMethod(isolate, context, mod, "isValid",          ColorIsValidCb);
    ok = ok && SetMethod(isolate, context, mod, "mix",              ColorMixCb);
    ok = ok && SetMethod(isolate, context, mod, "scaleBrightness",  ColorScaleBrightnessCb);
    ok = ok && SetMethod(isolate, context, mod, "withAlpha",        ColorWithAlphaCb);
    if (!ok) return false;
    return ExportGlobalModule(isolate, context, "QtColor", mod);
}

}  // namespace qt6::gui
