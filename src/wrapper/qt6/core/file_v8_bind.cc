#include "wrapper/qt6/core/file.h"

#include "wrapper/qt6/v8/class_builder.h"

namespace qt6::core::file_v8_detail {

void FileCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void FilePath(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileAbsPath(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileDirPath(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileBaseName(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileExtension(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileMimeType(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileExists(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileSize(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileIsSymLink(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileReadText(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileReadBytes(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileWriteText(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileAppendText(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileRemove(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileRename(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileCopy(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileReadTextAsync(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileReadBytesAsync(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileWriteTextAsync(const v8::FunctionCallbackInfo<v8::Value>& a);
void FileToString(const v8::FunctionCallbackInfo<v8::Value>& a);

}  // namespace qt6::core::file_v8_detail

namespace qt6::core {

using namespace qt6::v8bridge;

bool RegisterQtFileClass(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    auto tpl = MakeClass(isolate, "QtFile", file_v8_detail::FileCtor,
        {
            { "path", file_v8_detail::FilePath },
            { "absolutePath", file_v8_detail::FileAbsPath },
            { "dirPath", file_v8_detail::FileDirPath },
            { "baseName", file_v8_detail::FileBaseName },
            { "extension", file_v8_detail::FileExtension },
            { "mimeType", file_v8_detail::FileMimeType },
            { "exists", file_v8_detail::FileExists },
            { "size", file_v8_detail::FileSize },
            { "isSymLink", file_v8_detail::FileIsSymLink },
            { "readText", file_v8_detail::FileReadText },
            { "readBytes", file_v8_detail::FileReadBytes },
            { "writeText", file_v8_detail::FileWriteText },
            { "appendText", file_v8_detail::FileAppendText },
            { "remove", file_v8_detail::FileRemove },
            { "rename", file_v8_detail::FileRename },
            { "copy", file_v8_detail::FileCopy },
            { "readTextAsync", file_v8_detail::FileReadTextAsync },
            { "readBytesAsync", file_v8_detail::FileReadBytesAsync },
            { "writeTextAsync", file_v8_detail::FileWriteTextAsync },
            { "toString", file_v8_detail::FileToString },
        });

    return ExportClass(isolate, context, "QtFile", tpl);
}

}  // namespace qt6::core
