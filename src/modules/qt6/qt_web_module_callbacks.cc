#include "modules/qt6/qt_web_module.h"

#include "wrapper/qt6/web/page.h"
#include "wrapper/qt6/util/v8_string.h"

#ifndef ENGINE_HAS_QT6_WEBVIEW
#define ENGINE_HAS_QT6_WEBVIEW 0
#endif

#ifndef ENGINE_HAS_QT6_WEBCHANNEL
#define ENGINE_HAS_QT6_WEBCHANNEL 0
#endif

#ifndef ENGINE_HAS_QT6_WEBENGINE
#define ENGINE_HAS_QT6_WEBENGINE 0
#endif

namespace modules::qt_web_module_detail {

void WebIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(
        v8::Boolean::New(args.GetIsolate(), qt6::web::IsWebEngineAvailable()));
}

void WebEngineIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const bool ok = (ENGINE_HAS_QT6_WEBENGINE == 1) && qt6::web::IsWebEngineAvailable();
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok));
}

void WebViewIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_WEBVIEW == 1));
}

void WebChannelIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_QT6_WEBCHANNEL == 1));
}

void WebUserAgentCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto ua = qt6::web::GetUserAgent();
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(args.GetIsolate(), ua.c_str()).ToLocalChecked());
}

void WebCachePathCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto p = qt6::web::GetDefaultCachePath();
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(args.GetIsolate(), p.c_str()).ToLocalChecked());
}

void WebStoragePathCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    auto p = qt6::web::GetDefaultStoragePath();
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(args.GetIsolate(), p.c_str()).ToLocalChecked());
}

void WebLoadUrlCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) {
        iso->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(iso, "Web.loadUrl(url: string, timeout?: number)")));
        return;
    }
    auto url     = qt6::util::V8ValueToStdString(iso, args[0]);
    int  timeout = args.Length() >= 2
        ? args[1]->Int32Value(ctx).FromMaybe(10000)
        : 10000;

    auto result = qt6::web::LoadUrlSync(url, timeout);

    v8::Local<v8::Object> obj = v8::Object::New(iso);
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "ok"),
             v8::Boolean::New(iso, result.ok)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "status"),
             v8::Integer::New(iso, result.status)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "body"),
             v8::String::NewFromUtf8(iso, result.body.c_str()).ToLocalChecked()).Check();
    if (!result.error.empty()) {
        obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "error"),
                 v8::String::NewFromUtf8(iso, result.error.c_str()).ToLocalChecked()).Check();
    }
    args.GetReturnValue().Set(obj);
}

}  // namespace modules::qt_web_module_detail
