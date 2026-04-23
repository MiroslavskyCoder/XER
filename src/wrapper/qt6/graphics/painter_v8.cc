#include "wrapper/qt6/graphics/painter_v8.h"
#include "wrapper/qt6/graphics/painter.h"
#include "wrapper/qt6/util/v8_string.h"
#include "wrapper/qt6/v8/class_builder.h"
#include "wrapper/qt6/v8/module_builder.h"

namespace qt6::graphics::painter_v8_detail {

using namespace qt6::v8bridge;

PathBuilder* GetSelf(const v8::FunctionCallbackInfo<v8::Value>& a) {
    return UnwrapPointer<PathBuilder>(a.This());
}

void PathCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtPainterPath()'");
        return;
    }
    auto* w = new PathBuilder();
    WrapPointer(args.This(), w);
    RegisterWeakCleanup(args.GetIsolate(), args.This(), w);
    args.GetReturnValue().Set(args.This());
}

void PathMoveTo(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 2) return;
    auto ctx = a.GetIsolate()->GetCurrentContext();
    double x = a[0]->NumberValue(ctx).FromMaybe(0.0);
    double y = a[1]->NumberValue(ctx).FromMaybe(0.0);
    s->MoveTo(x, y);
    a.GetReturnValue().Set(a.This());
}

void PathLineTo(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 2) return;
    auto ctx = a.GetIsolate()->GetCurrentContext();
    double x = a[0]->NumberValue(ctx).FromMaybe(0.0);
    double y = a[1]->NumberValue(ctx).FromMaybe(0.0);
    s->LineTo(x, y);
    a.GetReturnValue().Set(a.This());
}

void PathCubicTo(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 6) return;
    auto ctx = a.GetIsolate()->GetCurrentContext();
    double cx1 = a[0]->NumberValue(ctx).FromMaybe(0.0);
    double cy1 = a[1]->NumberValue(ctx).FromMaybe(0.0);
    double cx2 = a[2]->NumberValue(ctx).FromMaybe(0.0);
    double cy2 = a[3]->NumberValue(ctx).FromMaybe(0.0);
    double ex  = a[4]->NumberValue(ctx).FromMaybe(0.0);
    double ey  = a[5]->NumberValue(ctx).FromMaybe(0.0);
    s->CubicTo(cx1, cy1, cx2, cy2, ex, ey);
    a.GetReturnValue().Set(a.This());
}

void PathQuadTo(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 4) return;
    auto ctx = a.GetIsolate()->GetCurrentContext();
    double cx = a[0]->NumberValue(ctx).FromMaybe(0.0);
    double cy = a[1]->NumberValue(ctx).FromMaybe(0.0);
    double ex = a[2]->NumberValue(ctx).FromMaybe(0.0);
    double ey = a[3]->NumberValue(ctx).FromMaybe(0.0);
    s->QuadTo(cx, cy, ex, ey);
    a.GetReturnValue().Set(a.This());
}

void PathArcTo(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 7) return;
    auto ctx  = a.GetIsolate()->GetCurrentContext();
    double rx    = a[0]->NumberValue(ctx).FromMaybe(0.0);
    double ry    = a[1]->NumberValue(ctx).FromMaybe(0.0);
    double angle = a[2]->NumberValue(ctx).FromMaybe(0.0);
    bool large   = a[3]->BooleanValue(a.GetIsolate());
    bool sweep   = a[4]->BooleanValue(a.GetIsolate());
    double ex    = a[5]->NumberValue(ctx).FromMaybe(0.0);
    double ey    = a[6]->NumberValue(ctx).FromMaybe(0.0);
    s->ArcTo(rx, ry, angle, large, sweep, ex, ey);
    a.GetReturnValue().Set(a.This());
}

void PathClose(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    s->Close();
    a.GetReturnValue().Set(a.This());
}

void PathRenderToPng(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 3) {
        ThrowTypeError(a.GetIsolate(), "renderToPng(outPath, width, height, stroke?, fill?, strokeWidth?)");
        return;
    }
    auto iso = a.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    std::string out_path  = qt6::util::V8ValueToStdString(iso, a[0]);
    int         width     = a[1]->Int32Value(ctx).FromMaybe(512);
    int         height    = a[2]->Int32Value(ctx).FromMaybe(512);
    std::string stroke    = a.Length() >= 4 ? qt6::util::V8ValueToStdString(iso, a[3]) : "#000000";
    std::string fill      = a.Length() >= 5 ? qt6::util::V8ValueToStdString(iso, a[4]) : "transparent";
    double stroke_width   = a.Length() >= 6 ? a[5]->NumberValue(ctx).FromMaybe(1.0) : 1.0;
    bool ok = s->RenderToPng(out_path, width, height, stroke, fill, stroke_width);
    a.GetReturnValue().Set(v8::Boolean::New(iso, ok));
}

void PathClear(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    s->commands.clear();
    a.GetReturnValue().Set(a.This());
}

void PathCommandCount(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    a.GetReturnValue().Set(v8::Integer::New(a.GetIsolate(),
        static_cast<int>(s->commands.size())));
}

void PathToString(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    std::string out = "QtPainterPath[" + std::to_string(s->commands.size()) + " cmd(s)]";
    a.GetReturnValue().Set(
        v8::String::NewFromUtf8(a.GetIsolate(), out.c_str()).ToLocalChecked());
}

// ---------------------------------------------------------------------------
// Static draw functions → QtDraw module
// ---------------------------------------------------------------------------
void DrawRectCb(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto iso = a.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    // drawRect(outPng, width, height, rectObj, fill?, stroke?, strokeWidth?)
    if (a.Length() < 4 || !a[3]->IsObject()) {
        ThrowTypeError(iso, "QtDraw.rect(outPng, width, height, {x,y,w,h}, fill?, stroke?, strokeWidth?)");
        return;
    }
    std::string out_png = qt6::util::V8ValueToStdString(iso, a[0]);
    int width  = a[1]->Int32Value(ctx).FromMaybe(512);
    int height = a[2]->Int32Value(ctx).FromMaybe(512);
    auto rect_obj = a[3].As<v8::Object>();
    auto get_num = [&](const char* key, double def) -> double {
        auto v = rect_obj->Get(ctx, v8::String::NewFromUtf8(iso, key).ToLocalChecked());
        if (!v.IsEmpty()) return v.ToLocalChecked()->NumberValue(ctx).FromMaybe(def);
        return def;
    };
    Rect r;
    r.x = get_num("x", 0.0);
    r.y = get_num("y", 0.0);
    r.w = get_num("w", static_cast<double>(width));
    r.h = get_num("h", static_cast<double>(height));
    std::string fill   = a.Length() >= 5 ? qt6::util::V8ValueToStdString(iso, a[4]) : "#ffffff";
    std::string stroke = a.Length() >= 6 ? qt6::util::V8ValueToStdString(iso, a[5]) : "#000000";
    double sw = a.Length() >= 7 ? a[6]->NumberValue(ctx).FromMaybe(1.0) : 1.0;
    bool ok = DrawRect(out_png, width, height, r, fill, stroke, sw);
    a.GetReturnValue().Set(v8::Boolean::New(iso, ok));
}

void DrawTextCb(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto iso = a.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    // drawText(outPng, width, height, text, x, y, color?, fontName?, fontSize?)
    if (a.Length() < 6) {
        ThrowTypeError(iso, "QtDraw.text(outPng, width, height, text, x, y, color?, fontName?, fontSize?)");
        return;
    }
    std::string out_png   = qt6::util::V8ValueToStdString(iso, a[0]);
    int         width     = a[1]->Int32Value(ctx).FromMaybe(512);
    int         height    = a[2]->Int32Value(ctx).FromMaybe(512);
    std::string text      = qt6::util::V8ValueToStdString(iso, a[3]);
    double      x         = a[4]->NumberValue(ctx).FromMaybe(0.0);
    double      y         = a[5]->NumberValue(ctx).FromMaybe(0.0);
    std::string color     = a.Length() >= 7 ? qt6::util::V8ValueToStdString(iso, a[6]) : "#000000";
    std::string font_name = a.Length() >= 8 ? qt6::util::V8ValueToStdString(iso, a[7]) : "Arial";
    int         font_size = a.Length() >= 9 ? a[8]->Int32Value(ctx).FromMaybe(12) : 12;
    bool ok = DrawText(out_png, width, height, text, x, y, color, font_name, font_size);
    a.GetReturnValue().Set(v8::Boolean::New(iso, ok));
}

}  // namespace qt6::graphics::painter_v8_detail

namespace qt6::graphics {

bool RegisterQtPainterClass(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    using namespace qt6::v8bridge;
    using namespace painter_v8_detail;

    // Register QtPainterPath class
    auto tpl = MakeClass(isolate, "QtPainterPath", PathCtor,
        {
            { "moveTo",       PathMoveTo        },
            { "lineTo",       PathLineTo        },
            { "cubicTo",      PathCubicTo       },
            { "quadTo",       PathQuadTo        },
            { "arcTo",        PathArcTo         },
            { "close",        PathClose         },
            { "renderToPng",  PathRenderToPng   },
            { "clear",        PathClear         },
            { "commandCount", PathCommandCount  },
            { "toString",     PathToString      },
        });
    if (!ExportClass(isolate, context, "QtPainterPath", tpl)) return false;

    // Register QtDraw module
    v8::HandleScope scope(isolate);
    auto mod = v8::Object::New(isolate);
    bool ok = true;
    ok = ok && SetMethod(isolate, context, mod, "rect", DrawRectCb);
    ok = ok && SetMethod(isolate, context, mod, "text", DrawTextCb);
    if (!ok) return false;
    return ExportGlobalModule(isolate, context, "QtDraw", mod);
}

}  // namespace qt6::graphics
