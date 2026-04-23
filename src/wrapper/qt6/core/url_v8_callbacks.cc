#include "wrapper/qt6/core/url.h"

#include "wrapper/qt6/v8/class_builder.h"
#include "wrapper/qt6/util/v8_string.h"

namespace qt6::core::url_v8_detail {

using namespace qt6::v8bridge;

UrlWrapper* GetSelf(const v8::FunctionCallbackInfo<v8::Value>& a) {
    return UnwrapPointer<UrlWrapper>(a.This());
}

void UrlCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtUrl(str)'");
        return;
    }
    std::string s;
    if (args.Length() > 0 && args[0]->IsString()) s = FromV8Str(args.GetIsolate(), args[0]);
    auto* w = new UrlWrapper(s);
    WrapPointer(args.This(), w);
    RegisterWeakCleanup(args.GetIsolate(), args.This(), w);
    args.GetReturnValue().Set(args.This());
}

#define URL_STR_GETTER(Method, Accessor) \
void Url##Method(const v8::FunctionCallbackInfo<v8::Value>& a) { \
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), s->Accessor())); }

#define URL_BOOL_GETTER(Method, Accessor) \
void Url##Method(const v8::FunctionCallbackInfo<v8::Value>& a) { \
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->Accessor())); }

URL_BOOL_GETTER(IsValid, isValid)
URL_BOOL_GETTER(IsEmpty, isEmpty)
URL_STR_GETTER(ToString, toString)
URL_STR_GETTER(Scheme, scheme)
URL_STR_GETTER(Host, host)
URL_STR_GETTER(Path, path)
URL_STR_GETTER(Query, query)
URL_STR_GETTER(Fragment, fragment)
URL_STR_GETTER(UserInfo, userInfo)
URL_STR_GETTER(Authority, authority)
URL_STR_GETTER(ToLocalFile, toLocalFile)

void UrlPort(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(v8::Integer::New(a.GetIsolate(), s->port()));
}

#define URL_STR_SETTER(Method, Mutator) \
void Url##Method(const v8::FunctionCallbackInfo<v8::Value>& a) { \
    auto* s = GetSelf(a); if (!s || a.Length() < 1) return; \
    s->Mutator(FromV8Str(a.GetIsolate(), a[0])); }

URL_STR_SETTER(SetScheme, setScheme)
URL_STR_SETTER(SetHost, setHost)
URL_STR_SETTER(SetPath, setPath)
URL_STR_SETTER(SetQuery, setQuery)
URL_STR_SETTER(SetFragment, setFragment)

void UrlSetPort(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    s->setPort(a[0]->Int32Value(a.GetIsolate()->GetCurrentContext()).FromMaybe(-1));
}

void UrlResolved(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    auto resolved = s->resolved(FromV8Str(a.GetIsolate(), a[0]));
    a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), resolved.toString()));
}

void UrlStaticFromLocalFile(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (a.Length() < 1) return;
    auto url = UrlWrapper::fromLocalFile(FromV8Str(a.GetIsolate(), a[0]));
    a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), url.toString()));
}

#undef URL_STR_GETTER
#undef URL_BOOL_GETTER
#undef URL_STR_SETTER

}  // namespace qt6::core::url_v8_detail
