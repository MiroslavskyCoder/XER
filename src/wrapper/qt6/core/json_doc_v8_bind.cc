#include "wrapper/qt6/core/json_doc.h"

#include "wrapper/qt6/v8/module_builder.h"

namespace qt6::core::json_doc_v8_detail {

void JsonParse(const v8::FunctionCallbackInfo<v8::Value>& a);
void JsonStringify(const v8::FunctionCallbackInfo<v8::Value>& a);
void JsonParseFile(const v8::FunctionCallbackInfo<v8::Value>& a);
void JsonWriteFile(const v8::FunctionCallbackInfo<v8::Value>& a);
void JsonIsValid(const v8::FunctionCallbackInfo<v8::Value>& a);

}  // namespace qt6::core::json_doc_v8_detail

namespace qt6::core {

bool RegisterQtJsonModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope scope(isolate);

    using namespace qt6::v8bridge;
    auto mod = v8::Object::New(isolate);
    SetMethod(isolate, context, mod, "parse", json_doc_v8_detail::JsonParse);
    SetMethod(isolate, context, mod, "stringify", json_doc_v8_detail::JsonStringify);
    SetMethod(isolate, context, mod, "parseFile", json_doc_v8_detail::JsonParseFile);
    SetMethod(isolate, context, mod, "writeFile", json_doc_v8_detail::JsonWriteFile);
    SetMethod(isolate, context, mod, "isValid", json_doc_v8_detail::JsonIsValid);

    return ExportGlobalModule(isolate, context, "QtJson", mod);
}

}  // namespace qt6::core
