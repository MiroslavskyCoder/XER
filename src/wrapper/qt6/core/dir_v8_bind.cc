#include "wrapper/qt6/core/dir.h"

#include "wrapper/qt6/v8/class_builder.h"

namespace qt6::core::dir_v8_detail {

void DirCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void DirPath(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirAbsolutePath(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirCanonicalPath(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirDirName(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirExists(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirIsRoot(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirIsRelative(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirIsAbsolute(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirIsReadable(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirCd(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirCdUp(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirMkdir(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirMkpath(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirRmdir(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirRemoveRecursively(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirRename(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirRemove(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirEntryList(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirEntryListDirs(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirEntryListFiles(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirToString(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirStaticHome(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirStaticCurrent(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirStaticTemp(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirStaticSetCurrent(const v8::FunctionCallbackInfo<v8::Value>& a);
void DirStaticDrives(const v8::FunctionCallbackInfo<v8::Value>& a);

}  // namespace qt6::core::dir_v8_detail

namespace qt6::core {

using namespace qt6::v8bridge;

bool RegisterQtDirClass(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    auto tpl = MakeClass(isolate, "QtDir", dir_v8_detail::DirCtor,
        {
            { "path", dir_v8_detail::DirPath },
            { "absolutePath", dir_v8_detail::DirAbsolutePath },
            { "canonicalPath", dir_v8_detail::DirCanonicalPath },
            { "dirName", dir_v8_detail::DirDirName },
            { "exists", dir_v8_detail::DirExists },
            { "isRoot", dir_v8_detail::DirIsRoot },
            { "isRelative", dir_v8_detail::DirIsRelative },
            { "isAbsolute", dir_v8_detail::DirIsAbsolute },
            { "isReadable", dir_v8_detail::DirIsReadable },
            { "cd", dir_v8_detail::DirCd },
            { "cdUp", dir_v8_detail::DirCdUp },
            { "mkdir", dir_v8_detail::DirMkdir },
            { "mkpath", dir_v8_detail::DirMkpath },
            { "rmdir", dir_v8_detail::DirRmdir },
            { "removeRecursively", dir_v8_detail::DirRemoveRecursively },
            { "rename", dir_v8_detail::DirRename },
            { "remove", dir_v8_detail::DirRemove },
            { "entryList", dir_v8_detail::DirEntryList },
            { "entryListDirs", dir_v8_detail::DirEntryListDirs },
            { "entryListFiles", dir_v8_detail::DirEntryListFiles },
            { "toString", dir_v8_detail::DirToString },
        },
        {
            { "home", dir_v8_detail::DirStaticHome },
            { "current", dir_v8_detail::DirStaticCurrent },
            { "temp", dir_v8_detail::DirStaticTemp },
            { "setCurrent", dir_v8_detail::DirStaticSetCurrent },
            { "drives", dir_v8_detail::DirStaticDrives },
        });

    return ExportClass(isolate, context, "QtDir", tpl);
}

}  // namespace qt6::core
