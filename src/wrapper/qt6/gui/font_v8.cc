#include "wrapper/qt6/gui/font_v8.h"
#include "wrapper/qt6/gui/font.h"
#include "wrapper/qt6/util/v8_string.h"
#include "wrapper/qt6/v8/class_builder.h"
#include "wrapper/qt6/v8/module_builder.h"

namespace {

using namespace qt6::v8bridge;

void ThrowLocalTypeError(v8::Isolate* iso, const char* message) {
    iso->ThrowException(v8::Exception::TypeError(
        v8::String::NewFromUtf8(iso, message).ToLocalChecked()));
}

void FontParseCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) {
        ThrowLocalTypeError(iso, "QtFont.parse(family, size?, bold?, italic?)");
        return;
    }
    std::string family = qt6::util::V8ValueToStdString(iso, args[0]);
    int  size   = 12;
    bool bold   = false;
    bool italic = false;
    if (args.Length() >= 2 && args[1]->IsNumber())
        size = args[1]->Int32Value(ctx).FromMaybe(12);
    if (args.Length() >= 3)
        bold = args[2]->BooleanValue(iso);
    if (args.Length() >= 4)
        italic = args[3]->BooleanValue(iso);

    auto f = qt6::gui::ParseFont(family, size, bold, italic);
    auto obj = v8::Object::New(iso);
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "family"),
             v8::String::NewFromUtf8(iso, f.family.c_str()).ToLocalChecked()).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "pointSize"),
             v8::Integer::New(iso, f.point_size)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "bold"),
             v8::Boolean::New(iso, f.bold)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "italic"),
             v8::Boolean::New(iso, f.italic)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "weight"),
             v8::Integer::New(iso, f.weight)).Check();
    args.GetReturnValue().Set(obj);
}

void FontToStringCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsObject()) {
        ThrowLocalTypeError(iso, "QtFont.toString(fontObj)");
        return;
    }
    auto obj = args[0].As<v8::Object>();
    qt6::gui::FontDesc f;
    auto read_str = [&](const char* key) -> std::string {
        auto k = v8::String::NewFromUtf8(iso, key).ToLocalChecked();
        auto v = obj->Get(ctx, k);
        if (!v.IsEmpty() && v.ToLocalChecked()->IsString())
            return qt6::util::V8ValueToStdString(iso, v.ToLocalChecked());
        return {};
    };
    auto read_int = [&](const char* key, int def) -> int {
        auto k = v8::String::NewFromUtf8(iso, key).ToLocalChecked();
        auto v = obj->Get(ctx, k);
        if (!v.IsEmpty() && v.ToLocalChecked()->IsNumber())
            return v.ToLocalChecked()->Int32Value(ctx).FromMaybe(def);
        return def;
    };
    auto read_bool = [&](const char* key) -> bool {
        auto k = v8::String::NewFromUtf8(iso, key).ToLocalChecked();
        auto v = obj->Get(ctx, k);
        if (!v.IsEmpty()) return v.ToLocalChecked()->BooleanValue(iso);
        return false;
    };
    f.family     = read_str("family");
    f.point_size = read_int("pointSize", 12);
    f.bold       = read_bool("bold");
    f.italic     = read_bool("italic");
    f.weight     = read_int("weight", 400);
    auto s = qt6::gui::FontToString(f);
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, s.c_str()).ToLocalChecked());
}

}  // namespace

namespace qt6::gui {

bool RegisterQtFontModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope scope(isolate);
    using namespace qt6::v8bridge;

    auto mod = v8::Object::New(isolate);
    bool ok = true;
    ok = ok && SetMethod(isolate, context, mod, "parse",    FontParseCb);
    ok = ok && SetMethod(isolate, context, mod, "toString", FontToStringCb);
    if (!ok) return false;
    return ExportGlobalModule(isolate, context, "QtFont", mod);
}

}  // namespace qt6::gui
