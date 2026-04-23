#include "modules/qt6/qt_pdf_module.h"

#include "wrapper/qt6/pdf/document.h"
#include "wrapper/qt6/util/v8_string.h"

namespace modules::qt_pdf_module_detail {

void PdfLoadCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 1 || !args[0]->IsString()) {
        iso->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(iso, "Pdf.load(path: string)")));
        return;
    }
    auto info = qt6::pdf::LoadDocument(qt6::util::V8ValueToStdString(iso, args[0]));

    v8::Local<v8::Object> obj = v8::Object::New(iso);
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "ok"),
             v8::Boolean::New(iso, info.ok)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "pageCount"),
             v8::Integer::New(iso, info.page_count)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "title"),
             v8::String::NewFromUtf8(iso, info.title.c_str()).ToLocalChecked()).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "author"),
             v8::String::NewFromUtf8(iso, info.author.c_str()).ToLocalChecked()).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "subject"),
             v8::String::NewFromUtf8(iso, info.subject.c_str()).ToLocalChecked()).Check();
    if (!info.ok) {
        obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "error"),
                 v8::String::NewFromUtf8(iso, info.error.c_str()).ToLocalChecked()).Check();
    }
    args.GetReturnValue().Set(obj);
}

void PdfPageInfoCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 2) {
        iso->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(iso, "Pdf.pageInfo(path: string, pageIndex: number)")));
        return;
    }
    auto path  = qt6::util::V8ValueToStdString(iso, args[0]);
    int  index = args[1]->Int32Value(ctx).FromMaybe(0);
    auto pi    = qt6::pdf::GetPageInfo(path, index);

    v8::Local<v8::Object> obj = v8::Object::New(iso);
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "index"),
             v8::Integer::New(iso, pi.index)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "width"),
             v8::Number::New(iso, pi.width)).Check();
    obj->Set(ctx, v8::String::NewFromUtf8Literal(iso, "height"),
             v8::Number::New(iso, pi.height)).Check();
    args.GetReturnValue().Set(obj);
}

void PdfRenderCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 3) {
        iso->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(iso,
                "Pdf.renderPage(pdfPath, pageIndex, outPng, scale?)")));
        return;
    }
    auto pdf   = qt6::util::V8ValueToStdString(iso, args[0]);
    int  idx   = args[1]->Int32Value(ctx).FromMaybe(0);
    auto out   = qt6::util::V8ValueToStdString(iso, args[2]);
    double sc  = args.Length() >= 4 ? args[3]->NumberValue(ctx).FromMaybe(1.0) : 1.0;
    bool ok    = qt6::pdf::RenderPageToPng(pdf, idx, out, sc);
    args.GetReturnValue().Set(v8::Boolean::New(iso, ok));
}

void PdfExtractTextCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* iso = args.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    if (args.Length() < 2) {
        iso->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(iso, "Pdf.extractText(path, pageIndex)")));
        return;
    }
    auto path = qt6::util::V8ValueToStdString(iso, args[0]);
    int  idx  = args[1]->Int32Value(ctx).FromMaybe(0);
    auto text = qt6::pdf::ExtractPageText(path, idx);
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(iso, text.c_str()).ToLocalChecked());
}

void PdfIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args) {
#if defined(QT_PDF_LIB)
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), true));
#else
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), false));
#endif
}

}  // namespace modules::qt_pdf_module_detail
