#include "wrapper/qt6/core/dir.h"

#include "wrapper/qt6/v8/class_builder.h"
#include "wrapper/qt6/util/v8_string.h"

namespace qt6::core::dir_v8_detail {

using namespace qt6::v8bridge;

DirWrapper* GetSelf(const v8::FunctionCallbackInfo<v8::Value>& args) {
    return UnwrapPointer<DirWrapper>(args.This());
}

void DirCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtDir(path)'");
        return;
    }
    std::string p = ".";
    if (args.Length() > 0 && args[0]->IsString()) p = FromV8Str(args.GetIsolate(), args[0]);

    auto* w = new DirWrapper(p);
    WrapPointer(args.This(), w);
    RegisterWeakCleanup(args.GetIsolate(), args.This(), w);
    args.GetReturnValue().Set(args.This());
}

void DirPath(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), s->path()));
}
void DirAbsolutePath(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), s->absolutePath()));
}
void DirCanonicalPath(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), s->canonicalPath()));
}
void DirDirName(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), s->dirName()));
}
void DirExists(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    bool result;
    if (a.Length() > 0 && a[0]->IsString()) result = s->exists(FromV8Str(a.GetIsolate(), a[0]));
    else result = s->exists();
    a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), result));
}
void DirIsRoot(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->isRoot()));
}
void DirIsRelative(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->isRelative()));
}
void DirIsAbsolute(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->isAbsolute()));
}
void DirIsReadable(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->isReadable()));
}
void DirCd(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    if (a.Length() < 1 || !a[0]->IsString()) {
        ThrowTypeError(a.GetIsolate(), "cd(path)");
        return;
    }
    a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->cd(FromV8Str(a.GetIsolate(), a[0]))));
}
void DirCdUp(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->cdUp()));
}
void DirMkdir(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->mkdir(FromV8Str(a.GetIsolate(), a[0]))));
}
void DirMkpath(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->mkpath(FromV8Str(a.GetIsolate(), a[0]))));
}
void DirRmdir(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->rmdir(FromV8Str(a.GetIsolate(), a[0]))));
}
void DirRemoveRecursively(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->removeRecursively()));
}
void DirRename(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 2) {
        ThrowTypeError(a.GetIsolate(), "rename(old, new)");
        return;
    }
    a.GetReturnValue().Set(v8::Boolean::New(
        a.GetIsolate(),
        s->rename(FromV8Str(a.GetIsolate(), a[0]), FromV8Str(a.GetIsolate(), a[1]))));
}
void DirRemove(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->remove(FromV8Str(a.GetIsolate(), a[0]))));
}

void DirEntryList(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    auto iso = a.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    std::string filter = "*";
    bool inc_dirs = true;
    bool inc_hidden = false;
    if (a.Length() > 0 && a[0]->IsString()) filter = FromV8Str(iso, a[0]);
    if (a.Length() > 1) inc_dirs = a[1]->BooleanValue(iso);
    if (a.Length() > 2) inc_hidden = a[2]->BooleanValue(iso);
    auto list = s->entryList(filter, inc_dirs, inc_hidden);
    auto arr = v8::Array::New(iso, static_cast<int>(list.size()));
    for (size_t i = 0; i < list.size(); ++i) {
        arr->Set(ctx, static_cast<uint32_t>(i), ToV8Str(iso, list[i])).Check();
    }
    a.GetReturnValue().Set(arr);
}
void DirEntryListDirs(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    auto iso = a.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    bool hidden = a.Length() > 0 && a[0]->BooleanValue(iso);
    auto list = s->entryListDirs(hidden);
    auto arr = v8::Array::New(iso, static_cast<int>(list.size()));
    for (size_t i = 0; i < list.size(); ++i) {
        arr->Set(ctx, static_cast<uint32_t>(i), ToV8Str(iso, list[i])).Check();
    }
    a.GetReturnValue().Set(arr);
}
void DirEntryListFiles(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    auto iso = a.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    std::string filter = "*";
    if (a.Length() > 0 && a[0]->IsString()) filter = FromV8Str(iso, a[0]);
    auto list = s->entryListFiles(filter);
    auto arr = v8::Array::New(iso, static_cast<int>(list.size()));
    for (size_t i = 0; i < list.size(); ++i) {
        arr->Set(ctx, static_cast<uint32_t>(i), ToV8Str(iso, list[i])).Check();
    }
    a.GetReturnValue().Set(arr);
}
void DirToString(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), "QtDir(" + s->absolutePath() + ")"));
}

void DirStaticHome(const v8::FunctionCallbackInfo<v8::Value>& a) {
    a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), DirWrapper::homePath()));
}
void DirStaticCurrent(const v8::FunctionCallbackInfo<v8::Value>& a) {
    a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), DirWrapper::currentPath()));
}
void DirStaticTemp(const v8::FunctionCallbackInfo<v8::Value>& a) {
    a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), DirWrapper::tempPath()));
}
void DirStaticSetCurrent(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (a.Length() < 1) return;
    a.GetReturnValue().Set(
        v8::Boolean::New(a.GetIsolate(), DirWrapper::setCurrent(FromV8Str(a.GetIsolate(), a[0]))));
}
void DirStaticDrives(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto iso = a.GetIsolate();
    auto ctx = iso->GetCurrentContext();
    auto drives = DirWrapper::drives();
    auto arr = v8::Array::New(iso, static_cast<int>(drives.size()));
    for (size_t i = 0; i < drives.size(); ++i) {
        arr->Set(ctx, static_cast<uint32_t>(i), ToV8Str(iso, drives[i])).Check();
    }
    a.GetReturnValue().Set(arr);
}

}  // namespace qt6::core::dir_v8_detail
