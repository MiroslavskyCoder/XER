#include "wrapper/qt6/web/web_page.h"

#include "wrapper/qt6/v8/class_builder.h"

namespace qt6::web::web_page_v8_detail {

void PageCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageLoad(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageSetHtml(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageHtml(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageText(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageTitle(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageUrl(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageSource(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageSetSource(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageReload(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageStop(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageCanGoBack(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageCanGoForward(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageBack(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageForward(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageHistoryCount(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageRunJavaScript(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageSetZoomFactor(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageZoomFactor(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageSetUserAgent(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageUserAgent(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageSetCachePath(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageSetStoragePath(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageSetOffTheRecord(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageIsOffTheRecord(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageStaticIsAvailable(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageStaticDefaultCachePath(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageStaticDefaultStoragePath(const v8::FunctionCallbackInfo<v8::Value>& args);
void PageStaticDefaultUserAgent(const v8::FunctionCallbackInfo<v8::Value>& args);

}  // namespace qt6::web::web_page_v8_detail

namespace qt6::web {

using namespace qt6::v8bridge;

bool RegisterQtWebPageClass(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    auto tpl = MakeClass(isolate, "QtWebPage", web_page_v8_detail::PageCtor,
        {
            { "load",             web_page_v8_detail::PageLoad },
            { "setHtml",          web_page_v8_detail::PageSetHtml },
            { "html",             web_page_v8_detail::PageHtml },
            { "text",             web_page_v8_detail::PageText },
            { "title",            web_page_v8_detail::PageTitle },
            { "url",              web_page_v8_detail::PageUrl },
            { "source",           web_page_v8_detail::PageSource },
            { "setSource",        web_page_v8_detail::PageSetSource },
            { "reload",           web_page_v8_detail::PageReload },
            { "stop",             web_page_v8_detail::PageStop },
            { "canGoBack",        web_page_v8_detail::PageCanGoBack },
            { "canGoForward",     web_page_v8_detail::PageCanGoForward },
            { "back",             web_page_v8_detail::PageBack },
            { "forward",          web_page_v8_detail::PageForward },
            { "historyCount",     web_page_v8_detail::PageHistoryCount },
            { "runJavaScript",    web_page_v8_detail::PageRunJavaScript },
            { "setZoomFactor",    web_page_v8_detail::PageSetZoomFactor },
            { "zoomFactor",       web_page_v8_detail::PageZoomFactor },
            { "setUserAgent",     web_page_v8_detail::PageSetUserAgent },
            { "userAgent",        web_page_v8_detail::PageUserAgent },
            { "setCachePath",     web_page_v8_detail::PageSetCachePath },
            { "setStoragePath",   web_page_v8_detail::PageSetStoragePath },
            { "setOffTheRecord",  web_page_v8_detail::PageSetOffTheRecord },
            { "isOffTheRecord",   web_page_v8_detail::PageIsOffTheRecord },
        },
        {
            { "isAvailable",        web_page_v8_detail::PageStaticIsAvailable },
            { "defaultCachePath",   web_page_v8_detail::PageStaticDefaultCachePath },
            { "defaultStoragePath", web_page_v8_detail::PageStaticDefaultStoragePath },
            { "defaultUserAgent",   web_page_v8_detail::PageStaticDefaultUserAgent },
        });

    return ExportClass(isolate, context, "QtWebPage", tpl);
}

}  // namespace qt6::web
