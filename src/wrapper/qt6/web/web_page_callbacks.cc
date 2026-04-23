#include "wrapper/qt6/web/web_page.h"

#include "wrapper/qt6/v8/class_builder.h"
#include "wrapper/qt6/web/page.h"
#include "wrapper/qt6/web/web_page_runtime.h"

#ifndef ENGINE_HAS_QT6
#define ENGINE_HAS_QT6 0
#endif

#if HAS_QT_WEBENGINE
#include <QEventLoop>
#include <QTimer>
#include <QUrl>
#include <QWebEngineHistory>
#endif

namespace qt6::web::web_page_v8_detail {

using namespace qt6::v8bridge;

void ThrowUnavailable(v8::Isolate* isolate) {
    ThrowError(isolate, "Qt6 WebEngine unavailable");
}

#if HAS_QT_WEBENGINE
WebPageWrapper* GetSelf(const v8::FunctionCallbackInfo<v8::Value>& args) {
    return UnwrapPointer<WebPageWrapper>(args.This());
}
#endif

void PageCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtWebPage(opts?)'");
        return;
    }

    auto* self = new WebPageWrapper();

    if (args.Length() >= 1 && args[0]->IsObject()) {
        auto iso = args.GetIsolate();
        auto ctx = iso->GetCurrentContext();
        auto obj = args[0].As<v8::Object>();

        auto key_off = ToV8Str(iso, "offTheRecord");
        auto key_ua = ToV8Str(iso, "userAgent");
        auto key_cp = ToV8Str(iso, "cachePath");
        auto key_sp = ToV8Str(iso, "storagePath");

        if (obj->Has(ctx, key_off).FromMaybe(false)) {
            bool off = obj->Get(ctx, key_off).ToLocalChecked()->BooleanValue(iso);
            self->profile()->setOffTheRecord(off);
        }
        if (obj->Has(ctx, key_ua).FromMaybe(false)) {
            auto ua = FromV8Str(iso, obj->Get(ctx, key_ua).ToLocalChecked());
            self->profile()->setHttpUserAgent(QString::fromUtf8(ua.c_str()));
        }
        if (obj->Has(ctx, key_cp).FromMaybe(false)) {
            auto p = FromV8Str(iso, obj->Get(ctx, key_cp).ToLocalChecked());
            self->profile()->setCachePath(QString::fromUtf8(p.c_str()));
        }
        if (obj->Has(ctx, key_sp).FromMaybe(false)) {
            auto p = FromV8Str(iso, obj->Get(ctx, key_sp).ToLocalChecked());
            self->profile()->setPersistentStoragePath(QString::fromUtf8(p.c_str()));
        }
    }

    WrapPointer(args.This(), self);
    RegisterWeakCleanup(args.GetIsolate(), args.This(), self);
    args.GetReturnValue().Set(args.This());
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageLoad(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1 || !args[0]->IsString()) return;

    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    int timeout = (args.Length() >= 2 && args[1]->IsNumber())
        ? args[1]->Int32Value(ctx).FromMaybe(10000)
        : 10000;

    auto result = LoadOnPageSync(self->page(), FromV8Str(iso, args[0]), timeout);

    auto obj = v8::Object::New(iso);
    obj->Set(ctx, ToV8Str(iso, "ok"), v8::Boolean::New(iso, result.ok)).Check();
    obj->Set(ctx, ToV8Str(iso, "status"), v8::Integer::New(iso, result.status)).Check();
    obj->Set(ctx, ToV8Str(iso, "body"), ToV8Str(iso, result.body)).Check();
    if (!result.error.empty()) {
        obj->Set(ctx, ToV8Str(iso, "error"), ToV8Str(iso, result.error)).Check();
    }
    args.GetReturnValue().Set(obj);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageSetHtml(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1 || !args[0]->IsString()) return;

    auto iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    int timeout = (args.Length() >= 3 && args[2]->IsNumber())
        ? args[2]->Int32Value(ctx).FromMaybe(5000)
        : 5000;

    std::string html = FromV8Str(iso, args[0]);
    std::string base;
    if (args.Length() >= 2 && args[1]->IsString()) {
        base = FromV8Str(iso, args[1]);
    }

    bool ok = false;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setInterval(timeout);

    QObject::connect(self->page(), &QWebEnginePage::loadFinished, [&](bool loaded) {
        ok = loaded;
        loop.quit();
    });
    QObject::connect(&timer, &QTimer::timeout, [&]() { loop.quit(); });

    self->page()->setHtml(QString::fromUtf8(html.c_str()), QUrl(QString::fromUtf8(base.c_str())));
    timer.start();
    loop.exec();

    args.GetReturnValue().Set(v8::Boolean::New(iso, ok));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageHtml(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    int timeout = (args.Length() >= 1 && args[0]->IsNumber())
        ? args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(5000)
        : 5000;
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), PageToHtml(self->page(), timeout)));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageText(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    int timeout = (args.Length() >= 1 && args[0]->IsNumber())
        ? args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(5000)
        : 5000;
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), PageToPlainText(self->page(), timeout)));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageTitle(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), self->page()->title().toUtf8().constData()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageUrl(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), self->page()->url().toString().toUtf8().constData()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageReload(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    self->page()->triggerAction(QWebEnginePage::Reload);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageStop(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    self->page()->triggerAction(QWebEnginePage::Stop);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageCanGoBack(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), self->page()->history()->canGoBack()));
#else
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
#endif
}

void PageCanGoForward(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), self->page()->history()->canGoForward()));
#else
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
#endif
}

void PageBack(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    self->page()->triggerAction(QWebEnginePage::Back);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageForward(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    self->page()->triggerAction(QWebEnginePage::Forward);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageHistoryCount(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), self->page()->history()->count()));
#else
    args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), 0));
#endif
}

void PageSetZoomFactor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    double value = args[0]->NumberValue(args.GetIsolate()->GetCurrentContext()).FromMaybe(1.0);
    if (value < 0.25) value = 0.25;
    if (value > 5.0) value = 5.0;
    self->page()->setZoomFactor(value);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageZoomFactor(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), self->page()->zoomFactor()));
#else
    args.GetReturnValue().Set(v8::Number::New(args.GetIsolate(), 1.0));
#endif
}

void PageSource(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), self->page()->requestedUrl().toString().toUtf8().constData()));
#else
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), ""));
#endif
}

void PageSetSource(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    PageLoad(args);
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageRunJavaScript(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1 || !args[0]->IsString()) return;

    int timeout = (args.Length() >= 2 && args[1]->IsNumber())
        ? args[1]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(5000)
        : 5000;

    auto out = RunJsSync(self->page(), FromV8Str(args.GetIsolate(), args[0]), timeout);
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), out));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageSetUserAgent(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1 || !args[0]->IsString()) return;
    self->profile()->setHttpUserAgent(QString::fromUtf8(FromV8Str(args.GetIsolate(), args[0]).c_str()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageUserAgent(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), self->profile()->httpUserAgent().toUtf8().constData()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageSetCachePath(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1 || !args[0]->IsString()) return;
    self->profile()->setCachePath(QString::fromUtf8(FromV8Str(args.GetIsolate(), args[0]).c_str()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageSetStoragePath(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1 || !args[0]->IsString()) return;
    self->profile()->setPersistentStoragePath(QString::fromUtf8(FromV8Str(args.GetIsolate(), args[0]).c_str()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageSetOffTheRecord(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self || args.Length() < 1) return;
    self->profile()->setOffTheRecord(args[0]->BooleanValue(args.GetIsolate()));
#else
    (void)args;
    ThrowUnavailable(args.GetIsolate());
#endif
}

void PageIsOffTheRecord(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if HAS_QT_WEBENGINE
    auto* self = GetSelf(args);
    if (!self) return;
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), self->profile()->isOffTheRecord()));
#else
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
#endif
}

void PageStaticIsAvailable(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), IsWebEngineAvailable()));
}

void PageStaticDefaultCachePath(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), GetDefaultCachePath()));
}

void PageStaticDefaultStoragePath(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), GetDefaultStoragePath()));
}

void PageStaticDefaultUserAgent(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(ToV8Str(args.GetIsolate(), GetUserAgent()));
}

}  // namespace qt6::web::web_page_v8_detail
