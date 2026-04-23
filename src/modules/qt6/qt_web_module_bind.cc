#include "modules/qt6/qt_web_module.h"

#include "wrapper/qt6/web/web_page.h"
#include "wrapper/qt6/v8/module_builder.h"

namespace modules::qt_web_module_detail {

void WebIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void WebEngineIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void WebViewIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void WebChannelIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void WebUserAgentCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void WebCachePathCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void WebStoragePathCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void WebLoadUrlCb(const v8::FunctionCallbackInfo<v8::Value>& args);

}  // namespace modules::qt_web_module_detail

namespace modules {

bool RegisterQtWebModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope scope(isolate);
    using namespace qt6::v8bridge;

    v8::Local<v8::Object> mod = v8::Object::New(isolate);
    bool ok = true;
    ok = ok && SetMethod(isolate, context, mod, "isAvailable",  qt_web_module_detail::WebIsAvailableCb);
    ok = ok && SetMethod(isolate, context, mod, "userAgent",    qt_web_module_detail::WebUserAgentCb);
    ok = ok && SetMethod(isolate, context, mod, "cachePath",    qt_web_module_detail::WebCachePathCb);
    ok = ok && SetMethod(isolate, context, mod, "storagePath",  qt_web_module_detail::WebStoragePathCb);
    ok = ok && SetMethod(isolate, context, mod, "loadUrl",      qt_web_module_detail::WebLoadUrlCb);
    if (!ok) return false;
    if (!ExportGlobalModule(isolate, context, "QtWeb", mod)) return false;

    v8::Local<v8::Object> web_engine = v8::Object::New(isolate);
    ok = true;
    ok = ok && SetMethod(isolate, context, web_engine, "isAvailable", qt_web_module_detail::WebEngineIsAvailableCb);
    ok = ok && SetMethod(isolate, context, web_engine, "userAgent",   qt_web_module_detail::WebUserAgentCb);
    ok = ok && SetMethod(isolate, context, web_engine, "cachePath",   qt_web_module_detail::WebCachePathCb);
    ok = ok && SetMethod(isolate, context, web_engine, "storagePath", qt_web_module_detail::WebStoragePathCb);
    ok = ok && SetMethod(isolate, context, web_engine, "loadUrl",     qt_web_module_detail::WebLoadUrlCb);
    if (!ok) return false;
    if (!ExportGlobalModule(isolate, context, "QtWebEngine", web_engine)) return false;

    v8::Local<v8::Object> web_view = v8::Object::New(isolate);
    ok = true;
    ok = ok && SetMethod(isolate, context, web_view, "isAvailable", qt_web_module_detail::WebViewIsAvailableCb);
    ok = ok && SetMethod(isolate, context, web_view, "loadUrl",     qt_web_module_detail::WebLoadUrlCb);
    if (!ok) return false;
    if (!ExportGlobalModule(isolate, context, "QtWebView", web_view)) return false;

    v8::Local<v8::Object> web_channel = v8::Object::New(isolate);
    ok = true;
    ok = ok && SetMethod(isolate, context, web_channel, "isAvailable", qt_web_module_detail::WebChannelIsAvailableCb);
    if (!ok) return false;
    if (!ExportGlobalModule(isolate, context, "QtWebChannel", web_channel)) return false;

    if (!qt6::web::RegisterQtWebPageClass(isolate, context)) return false;
    return true;
}

}  // namespace modules
