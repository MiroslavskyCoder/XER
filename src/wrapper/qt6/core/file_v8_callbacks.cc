#include "wrapper/qt6/core/file.h"

#include "wrapper/qt6/v8/async_bridge.h"
#include "wrapper/qt6/util/v8_string.h"

#include "async_io/async_file_reader.h"
#include "async_io/async_file_writer.h"

#include <cstring>

namespace qt6::core::file_v8_detail {

using namespace qt6::v8bridge;

FileWrapper* GetSelf(const v8::FunctionCallbackInfo<v8::Value>& a) {
    return UnwrapPointer<FileWrapper>(a.This());
}

void FileCtor(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (!args.IsConstructCall()) {
        ThrowTypeError(args.GetIsolate(), "Use 'new QtFile(path)'");
        return;
    }
    if (args.Length() < 1 || !args[0]->IsString()) {
        ThrowTypeError(args.GetIsolate(), "QtFile requires a path string");
        return;
    }
    auto* w = new FileWrapper(FromV8Str(args.GetIsolate(), args[0]));
    WrapPointer(args.This(), w);
    RegisterWeakCleanup(args.GetIsolate(), args.This(), w);
    args.GetReturnValue().Set(args.This());
}

void FilePath(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), s->filePath()));
}
void FileAbsPath(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), s->absolutePath()));
}
void FileDirPath(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), s->dirPath()));
}
void FileBaseName(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), s->baseName()));
}
void FileExtension(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), s->extension()));
}
void FileMimeType(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), s->mimeType()));
}
void FileExists(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->exists()));
}
void FileSize(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) {
        a.GetReturnValue().Set(v8::Number::New(a.GetIsolate(), static_cast<double>(s->size())));
    }
}
void FileIsSymLink(const v8::FunctionCallbackInfo<v8::Value>& a) {
    if (auto* s = GetSelf(a)) a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), s->isSymLink()));
}

void FileReadText(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    std::string out, err;
    if (!s->readText(out, err)) {
        ThrowError(a.GetIsolate(), err);
        return;
    }
    a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), out));
}

void FileReadBytes(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    auto iso = a.GetIsolate();
    std::vector<uint8_t> buf;
    std::string err;
    if (!s->readBytes(buf, err)) {
        ThrowError(iso, err);
        return;
    }
    auto ab = v8::ArrayBuffer::New(iso, buf.size());
    if (!buf.empty()) std::memcpy(ab->GetBackingStore()->Data(), buf.data(), buf.size());
    a.GetReturnValue().Set(v8::Uint8Array::New(ab, 0, buf.size()));
}

void FileWriteText(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    std::string err;
    bool ok = s->writeText(FromV8Str(a.GetIsolate(), a[0]), err);
    if (!ok) {
        ThrowError(a.GetIsolate(), err);
        return;
    }
    a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), true));
}

void FileAppendText(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    std::string err;
    bool ok = s->appendText(FromV8Str(a.GetIsolate(), a[0]), err);
    if (!ok) {
        ThrowError(a.GetIsolate(), err);
        return;
    }
    a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), true));
}

void FileRemove(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    std::string err;
    bool ok = s->remove(err);
    if (!ok) {
        ThrowError(a.GetIsolate(), err);
        return;
    }
    a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), true));
}

void FileRename(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    std::string err;
    bool ok = s->rename(FromV8Str(a.GetIsolate(), a[0]), err);
    if (!ok) {
        ThrowError(a.GetIsolate(), err);
        return;
    }
    a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), true));
}

void FileCopy(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s || a.Length() < 1) return;
    std::string err;
    bool ok = s->copy(FromV8Str(a.GetIsolate(), a[0]), err);
    if (!ok) {
        ThrowError(a.GetIsolate(), err);
        return;
    }
    a.GetReturnValue().Set(v8::Boolean::New(a.GetIsolate(), true));
}

void FileReadTextAsync(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    auto iso = a.GetIsolate();
    if (a.Length() < 1 || !a[0]->IsFunction()) {
        ThrowTypeError(iso, "readTextAsync(callback: (err, text) => void)");
        return;
    }
    auto cb = a[0].As<v8::Function>();
    auto ctx = iso->GetCurrentContext();
    std::string path = s->filePath();

    qt6::async::PostAsync<std::string>(
        iso,
        cb,
        ctx,
        [path](std::string& out_error) -> std::string {
            IO::AsyncIO::AsyncFileReader reader;
            if (!reader.OpenFile(path)) {
                out_error = reader.GetLastError();
                return {};
            }
            int64_t sz = reader.GetFileSize();
            if (sz <= 0) return {};
            std::vector<uint8_t> buf;
            if (!reader.ReadSync(static_cast<size_t>(sz), buf)) {
                out_error = reader.GetLastError();
                return {};
            }
            return std::string(reinterpret_cast<const char*>(buf.data()), buf.size());
        },
        [](v8::Isolate* iso, const std::string& text) -> v8::Local<v8::Value> {
            return ToV8Str(iso, text);
        });
}

void FileReadBytesAsync(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    auto iso = a.GetIsolate();
    if (a.Length() < 1 || !a[0]->IsFunction()) {
        ThrowTypeError(iso, "readBytesAsync(callback: (err, Uint8Array) => void)");
        return;
    }
    auto cb = a[0].As<v8::Function>();
    auto ctx = iso->GetCurrentContext();
    std::string path = s->filePath();

    qt6::async::PostAsync<std::vector<uint8_t>>(
        iso,
        cb,
        ctx,
        [path](std::string& out_error) -> std::vector<uint8_t> {
            IO::AsyncIO::AsyncFileReader reader;
            if (!reader.OpenFile(path)) {
                out_error = reader.GetLastError();
                return {};
            }
            int64_t sz = reader.GetFileSize();
            if (sz <= 0) return {};
            std::vector<uint8_t> buf;
            if (!reader.ReadSync(static_cast<size_t>(sz), buf)) {
                out_error = reader.GetLastError();
                return {};
            }
            return buf;
        },
        [](v8::Isolate* iso, const std::vector<uint8_t>& data) -> v8::Local<v8::Value> {
            auto ab = v8::ArrayBuffer::New(iso, data.size());
            if (!data.empty()) std::memcpy(ab->GetBackingStore()->Data(), data.data(), data.size());
            return v8::Uint8Array::New(ab, 0, data.size());
        });
}

void FileWriteTextAsync(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    auto iso = a.GetIsolate();
    if (a.Length() < 2 || !a[0]->IsString() || !a[1]->IsFunction()) {
        ThrowTypeError(iso, "writeTextAsync(text: string, callback: (err, ok) => void)");
        return;
    }
    auto cb = a[1].As<v8::Function>();
    auto ctx = iso->GetCurrentContext();
    std::string path = s->filePath();
    std::string text = FromV8Str(iso, a[0]);

    qt6::async::PostAsync<bool>(
        iso,
        cb,
        ctx,
        [path, text](std::string& out_error) -> bool {
            IO::AsyncIO::AsyncFileWriter writer;
            if (!writer.CreateFile(path, false)) {
                out_error = writer.GetLastError();
                return false;
            }
            bool ok = writer.WriteSync(reinterpret_cast<const uint8_t*>(text.data()), text.size());
            if (!ok) out_error = writer.GetLastError();
            return ok;
        },
        [](v8::Isolate* iso, const bool& ok) -> v8::Local<v8::Value> {
            return v8::Boolean::New(iso, ok);
        });
}

void FileToString(const v8::FunctionCallbackInfo<v8::Value>& a) {
    auto* s = GetSelf(a);
    if (!s) return;
    a.GetReturnValue().Set(ToV8Str(a.GetIsolate(), "QtFile(" + s->filePath() + ")"));
}

}  // namespace qt6::core::file_v8_detail
