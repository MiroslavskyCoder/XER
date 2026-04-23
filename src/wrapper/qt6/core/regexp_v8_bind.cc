#include "wrapper/qt6/core/regexp.h"

#include "wrapper/qt6/v8/class_builder.h"

namespace qt6::core::regexp_v8_detail {

void ReCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void ReIsValid(const v8::FunctionCallbackInfo<v8::Value>& a);
void ReErrorString(const v8::FunctionCallbackInfo<v8::Value>& a);
void RePattern(const v8::FunctionCallbackInfo<v8::Value>& a);
void ReTest(const v8::FunctionCallbackInfo<v8::Value>& a);
void ReMatch(const v8::FunctionCallbackInfo<v8::Value>& a);
void ReMatchAll(const v8::FunctionCallbackInfo<v8::Value>& a);
void ReReplace(const v8::FunctionCallbackInfo<v8::Value>& a);
void ReReplaceAll(const v8::FunctionCallbackInfo<v8::Value>& a);
void ReSplit(const v8::FunctionCallbackInfo<v8::Value>& a);

}  // namespace qt6::core::regexp_v8_detail

namespace qt6::core {

using namespace qt6::v8bridge;

bool RegisterQtRegExpClass(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    auto tpl = MakeClass(isolate, "QtRegExp", regexp_v8_detail::ReCtor,
        {
            { "isValid", regexp_v8_detail::ReIsValid },
            { "errorString", regexp_v8_detail::ReErrorString },
            { "pattern", regexp_v8_detail::RePattern },
            { "test", regexp_v8_detail::ReTest },
            { "match", regexp_v8_detail::ReMatch },
            { "matchAll", regexp_v8_detail::ReMatchAll },
            { "replace", regexp_v8_detail::ReReplace },
            { "replaceAll", regexp_v8_detail::ReReplaceAll },
            { "split", regexp_v8_detail::ReSplit },
        });
    return ExportClass(isolate, context, "QtRegExp", tpl);
}

}  // namespace qt6::core
