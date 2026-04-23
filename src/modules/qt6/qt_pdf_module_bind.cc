#include "modules/qt6/qt_pdf_module.h"

#include "wrapper/qt6/v8/module_builder.h"

namespace modules::qt_pdf_module_detail {

void PdfIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void PdfLoadCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void PdfPageInfoCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void PdfRenderCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void PdfExtractTextCb(const v8::FunctionCallbackInfo<v8::Value>& args);

}  // namespace modules::qt_pdf_module_detail

namespace modules {

bool RegisterQtPdfModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope scope(isolate);
    using namespace qt6::v8bridge;

    v8::Local<v8::Object> mod = v8::Object::New(isolate);
    bool ok = true;
    ok = ok && SetMethod(isolate, context, mod, "isAvailable", qt_pdf_module_detail::PdfIsAvailableCb);
    ok = ok && SetMethod(isolate, context, mod, "load",        qt_pdf_module_detail::PdfLoadCb);
    ok = ok && SetMethod(isolate, context, mod, "pageInfo",    qt_pdf_module_detail::PdfPageInfoCb);
    ok = ok && SetMethod(isolate, context, mod, "renderPage",  qt_pdf_module_detail::PdfRenderCb);
    ok = ok && SetMethod(isolate, context, mod, "extractText", qt_pdf_module_detail::PdfExtractTextCb);
    if (!ok) return false;

    return ExportGlobalModule(isolate, context, "QtPdf", mod);
}

}  // namespace modules
