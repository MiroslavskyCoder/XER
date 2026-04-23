#include "wrapper/qt6/core/url.h"

#include "wrapper/qt6/v8/class_builder.h"

namespace qt6::core::url_v8_detail {

void UrlCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void UrlIsValid(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlIsEmpty(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlToString(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlScheme(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlHost(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlPort(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlPath(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlQuery(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlFragment(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlUserInfo(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlAuthority(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlToLocalFile(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlSetScheme(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlSetHost(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlSetPort(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlSetPath(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlSetQuery(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlSetFragment(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlResolved(const v8::FunctionCallbackInfo<v8::Value>& a);
void UrlStaticFromLocalFile(const v8::FunctionCallbackInfo<v8::Value>& a);

}  // namespace qt6::core::url_v8_detail

namespace qt6::core {

using namespace qt6::v8bridge;

bool RegisterQtUrlClass(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    auto tpl = MakeClass(isolate, "QtUrl", url_v8_detail::UrlCtor,
        {
            { "isValid", url_v8_detail::UrlIsValid },
            { "isEmpty", url_v8_detail::UrlIsEmpty },
            { "toString", url_v8_detail::UrlToString },
            { "scheme", url_v8_detail::UrlScheme },
            { "host", url_v8_detail::UrlHost },
            { "port", url_v8_detail::UrlPort },
            { "path", url_v8_detail::UrlPath },
            { "query", url_v8_detail::UrlQuery },
            { "fragment", url_v8_detail::UrlFragment },
            { "userInfo", url_v8_detail::UrlUserInfo },
            { "authority", url_v8_detail::UrlAuthority },
            { "toLocalFile", url_v8_detail::UrlToLocalFile },
            { "setScheme", url_v8_detail::UrlSetScheme },
            { "setHost", url_v8_detail::UrlSetHost },
            { "setPort", url_v8_detail::UrlSetPort },
            { "setPath", url_v8_detail::UrlSetPath },
            { "setQuery", url_v8_detail::UrlSetQuery },
            { "setFragment", url_v8_detail::UrlSetFragment },
            { "resolved", url_v8_detail::UrlResolved },
        },
        {
            { "fromLocalFile", url_v8_detail::UrlStaticFromLocalFile },
        });

    return ExportClass(isolate, context, "QtUrl", tpl);
}

}  // namespace qt6::core
