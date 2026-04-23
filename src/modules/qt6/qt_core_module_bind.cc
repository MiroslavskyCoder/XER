#include "modules/qt6/qt_core_module.h"

#include "wrapper/qt6/core/dir.h"
#include "wrapper/qt6/core/file.h"
#include "wrapper/qt6/core/json_doc.h"
#include "wrapper/qt6/core/process.h"
#include "wrapper/qt6/core/regexp.h"
#include "wrapper/qt6/core/url.h"
#include "wrapper/qt6/v8/module_builder.h"

#ifndef ENGINE_HAS_QT6_CORE
#define ENGINE_HAS_QT6_CORE 0
#endif

#if ENGINE_HAS_QT6_CORE
#include <Qt>
#endif

namespace modules::qt_core_module_detail {

void CoreIsAvailableCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void CoreVersionCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void CoreCleanPathCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void CoreNativeSepCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void CoreAbsolutePathCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void CoreDirNameCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void CoreBaseNameCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void CoreExtensionCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void CoreJoinPathCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void CorePathExistsCb(const v8::FunctionCallbackInfo<v8::Value>& args);
void CoreIsDirCb(const v8::FunctionCallbackInfo<v8::Value>& args);

}  // namespace modules::qt_core_module_detail

namespace modules {

bool RegisterQtCoreModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope scope(isolate);
    v8::Local<v8::Object> mod = v8::Object::New(isolate);

    using namespace qt6::v8bridge;
    bool ok = true;
    ok = ok && SetMethod(isolate, context, mod, "isAvailable",          qt_core_module_detail::CoreIsAvailableCb);
    ok = ok && SetMethod(isolate, context, mod, "version",              qt_core_module_detail::CoreVersionCb);
    ok = ok && SetMethod(isolate, context, mod, "cleanPath",            qt_core_module_detail::CoreCleanPathCb);
    ok = ok && SetMethod(isolate, context, mod, "toNativeSeparators",   qt_core_module_detail::CoreNativeSepCb);
    ok = ok && SetMethod(isolate, context, mod, "absolutePath",         qt_core_module_detail::CoreAbsolutePathCb);
    ok = ok && SetMethod(isolate, context, mod, "dirName",              qt_core_module_detail::CoreDirNameCb);
    ok = ok && SetMethod(isolate, context, mod, "baseName",             qt_core_module_detail::CoreBaseNameCb);
    ok = ok && SetMethod(isolate, context, mod, "extension",            qt_core_module_detail::CoreExtensionCb);
    ok = ok && SetMethod(isolate, context, mod, "joinPath",             qt_core_module_detail::CoreJoinPathCb);
    ok = ok && SetMethod(isolate, context, mod, "pathExists",           qt_core_module_detail::CorePathExistsCb);
    ok = ok && SetMethod(isolate, context, mod, "isDir",                qt_core_module_detail::CoreIsDirCb);

    if (!ok) return false;
    if (!ExportGlobalModule(isolate, context, "QtCore", mod)) return false;

        {
        auto qt_ns = v8::Object::New(isolate);
    #if ENGINE_HAS_QT6_CORE
        qt_ns->Set(context, qt6::v8bridge::ToV8Str(isolate, "AlignCenter"),
               v8::Integer::New(isolate, static_cast<int>(Qt::AlignCenter))).Check();
    #else
        qt_ns->Set(context, qt6::v8bridge::ToV8Str(isolate, "AlignCenter"),
               v8::Integer::New(isolate, 0)).Check();
    #endif
        mod->Set(context, qt6::v8bridge::ToV8Str(isolate, "Qt"), qt_ns).Check();
        }

    if (!qt6::core::RegisterQtDirClass(isolate, context))     return false;
    if (!qt6::core::RegisterQtFileClass(isolate, context))    return false;
    if (!qt6::core::RegisterQtProcessClass(isolate, context)) return false;
    if (!qt6::core::RegisterQtUrlClass(isolate, context))     return false;
    if (!qt6::core::RegisterQtRegExpClass(isolate, context))  return false;
    if (!qt6::core::RegisterQtJsonModule(isolate, context))   return false;

    return true;
}

}  // namespace modules
