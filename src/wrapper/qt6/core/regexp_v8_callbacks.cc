#include "wrapper/qt6/core/regexp.h"

#include "wrapper/qt6/v8/class_builder.h"
#include "wrapper/qt6/util/v8_string.h"

namespace qt6::core::regexp_v8_detail {

using namespace qt6::v8bridge;

RegExpWrapper* GetSelf(const v8::FunctionCallbackInfo<v8::Value>& a) {
    return UnwrapPointer<RegExpWrapper>(a.This());
}

v8::Local<v8::Object> MatchToV8(v8::Isolate* iso, const RegExpMatch& rm) {
    auto ctx = iso->GetCurrentContext();
    auto obj = v8::Object::New(iso);
    obj->Set(ctx, ToV8Str(iso, "matched"), v8::Boolean::New(iso, rm.matched)).Check();
    obj->Set(ctx, ToV8Str(iso, "offset"), v8::Integer::New(iso, rm.offset)).Check();
    obj->Set(ctx, ToV8Str(iso, "length"), v8::Integer::New(iso, rm.length)).Check();
    obj->Set(ctx, ToV8Str(iso, "captured"), ToV8Str(iso, rm.captured)).Check();
    auto groups = v8::Array::New(iso, static_cast<int>(rm.groups.size()));
    for (size_t i = 0; i < rm.groups.size(); ++i) {
        groups->Set(ctx, static_cast<uint32_t>(i), ToV8Str(iso, rm.groups[i])).Check();
    }
    obj->Set(ctx, ToV8Str(iso, "groups"), groups).Check();
    return obj;
}

void ReCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtRegExp(pattern, flags?)'");
        return;
    }
    auto iso = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        ThrowTypeError(iso, "QtRegExp(pattern: string, flags?: string)");
        return;
    }
    std::string pattern = FromV8Str(iso, args[0]);
    bool ci = false;
    bool ml = false;
    bool da = false;
    if (args.Length() >= 2 && args[1]->IsString()) {
        auto flags = FromV8Str(iso, args[1]);
        for (char c : flags) {
            if (c == 'i') ci = true;
            if (c == 'm') ml = true;
            if (c == 's') da = true;
        }
    }
    auto* w = new RegExpWrapper(pattern, !ci, ml, da);
    WrapPointer(args.This(), w);
    RegisterWeakCleanup(iso, args.This(), w);
    args.GetReturnValue().Set(args.This());
}

void ReIsValid(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->isValid()));
}
void ReErrorString(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), s->errorString()));
}
void RePattern(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), s->pattern()));
}
void ReTest(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->test(FromV8Str(a.GetIsolate(), a[0]))));
}
void ReMatch(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    auto iso = a.GetIsolate();
    int offset = a.Length() >= 2 ? a[1]->Int32Value(iso->GetCurrentContext()).FromMaybe(0) : 0;
    a.GetReturnValue().Set(MatchToV8(iso, s->match(FromV8Str(iso, a[0]), offset)));
}
void ReMatchAll(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    auto iso = a.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    auto matches = s->matchAll(FromV8Str(iso, a[0]));
    auto arr = v8::Array::New(iso, static_cast<int>(matches.size()));
    for (size_t i = 0; i < matches.size(); ++i) {
        arr->Set(ctx, static_cast<uint32_t>(i), MatchToV8(iso, matches[i])).Check();
    }
    a.GetReturnValue().Set(arr);
}
void ReReplace(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 2) return;
    auto iso = a.GetIsolate();
    auto result = s->replace(FromV8Str(iso, a[0]), FromV8Str(iso, a[1]));
    a.GetReturnValue().Set(ToV8Str(iso, result));
}
void ReReplaceAll(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 2) return;
    auto iso = a.GetIsolate();
    auto result = s->replaceAll(FromV8Str(iso, a[0]), FromV8Str(iso, a[1]));
    a.GetReturnValue().Set(ToV8Str(iso, result));
}
void ReSplit(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    auto iso = a.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    auto parts = s->split(FromV8Str(iso, a[0]));
    auto arr = v8::Array::New(iso, static_cast<int>(parts.size()));
    for (size_t i = 0; i < parts.size(); ++i) {
        arr->Set(ctx, static_cast<uint32_t>(i), ToV8Str(iso, parts[i])).Check();
    }
    a.GetReturnValue().Set(arr);
}

}  // namespace qt6::core::regexp_v8_detail
