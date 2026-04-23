#pragma once

#include <v8.h>

namespace qt6::web {

// Registers global class QtWebPage.
// JS API (instance):
//   new QtWebPage(opts?)
//   load(url, timeout?) -> { ok, status, body, error?, contentType? }
//   setHtml(html, baseUrl?, timeout?) -> bool
//   html(timeout?), text(timeout?)
//   title(), url(), source(), setSource(url, timeout?)
//   reload(), stop(), back(), forward()
//   canGoBack(), canGoForward(), historyCount()
//   runJavaScript(script, timeout?) -> string
//   setZoomFactor(v), zoomFactor()
//   setUserAgent(ua), userAgent()
//   setCachePath(path), setStoragePath(path)
//   setOffTheRecord(bool), isOffTheRecord()
// JS API (static):
//   QtWebPage.isAvailable()
//   QtWebPage.defaultCachePath()
//   QtWebPage.defaultStoragePath()
//   QtWebPage.defaultUserAgent()
bool RegisterQtWebPageClass(v8::Isolate* isolate, v8::Local<v8::Context> context);

}  // namespace qt6::web
