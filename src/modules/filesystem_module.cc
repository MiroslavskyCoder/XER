#include "modules/filesystem_module.h"

#include <algorithm>
#include <filesystem>
#include <fnmatch.h>
#include <string>
#include <vector>

#include "runtime_live.h"
#include "tool_to.h"

namespace {

std::string ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return *utf8;
}

void ThrowPathTypeError(v8::Isolate* isolate, const char* message) {
    isolate->ThrowException(v8::Exception::TypeError(
        v8::String::NewFromUtf8(isolate, message).ToLocalChecked()));
}

std::string NormalizePathPattern(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    return value;
}

bool WildcardMatch(const std::string& pattern, const std::string& candidate) {
    return fnmatch(pattern.c_str(), candidate.c_str(), 0) == 0;
}

bool MatchAnyPattern(const std::string& pattern,
                    const std::string& relative,
                    const std::string& absolute,
                    const std::string& filename) {
    if (WildcardMatch(pattern, relative)) {
        return true;
    }
    if (!relative.empty() && relative[0] != '.' && WildcardMatch(pattern, "./" + relative)) {
        return true;
    }
    if (WildcardMatch(pattern, absolute)) {
        return true;
    }
    return WildcardMatch(pattern, filename);
}

std::filesystem::path ComputeGlobRoot(const std::string& dir_pattern) {
    const std::string normalized = NormalizePathPattern(dir_pattern);
    const std::size_t wildcard_pos = normalized.find_first_of("*?[");
    if (wildcard_pos == std::string::npos) {
        std::filesystem::path path(normalized);
        if (path.has_parent_path()) {
            return path.parent_path();
        }
        return std::filesystem::current_path();
    }

    const std::size_t slash_pos = normalized.rfind('/', wildcard_pos);
    if (slash_pos == std::string::npos) {
        return std::filesystem::current_path();
    }

    const std::string root = normalized.substr(0, slash_pos);
    if (root.empty()) {
        return std::filesystem::path("/");
    }
    return std::filesystem::path(root);
}

bool ParseFilterPatterns(v8::Isolate* isolate,
                         v8::Local<v8::Context> context,
                         v8::Local<v8::Value> value,
                         std::vector<std::string>* out_patterns) {
    if (value->IsUndefined() || value->IsNull()) {
        return true;
    }

    if (value->IsString()) {
        out_patterns->push_back(NormalizePathPattern(ValueToString(isolate, value)));
        return true;
    }

    if (value->IsArray()) {
        v8::Local<v8::Array> array = value.As<v8::Array>();
        const uint32_t length = array->Length();
        for (uint32_t i = 0; i < length; ++i) {
            v8::Local<v8::Value> item;
            if (!array->Get(context, i).ToLocal(&item) || !item->IsString()) {
                return false;
            }
            out_patterns->push_back(NormalizePathPattern(ValueToString(isolate, item)));
        }
        return true;
    }

    return false;
}

bool BuildGlobResult(v8::Isolate* isolate,
                     v8::Local<v8::Context> context,
                     v8::Local<v8::Object> options,
                     v8::Local<v8::Array>* out_result) {
    v8::Local<v8::Value> dir_value;
    if (!options->Get(context, v8::String::NewFromUtf8Literal(isolate, "dir")).ToLocal(&dir_value) ||
        !dir_value->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Glob expects options.dir string")));
        return false;
    }

    const std::string dir_pattern = NormalizePathPattern(ValueToString(isolate, dir_value));
    std::vector<std::string> filters;
    v8::Local<v8::Value> filter_value;
    if (options->Get(context, v8::String::NewFromUtf8Literal(isolate, "filter")).ToLocal(&filter_value)) {
        if (!ParseFilterPatterns(isolate, context, filter_value, &filters)) {
            isolate->ThrowException(v8::Exception::TypeError(
                v8::String::NewFromUtf8Literal(isolate, "Glob options.filter must be string or string[]")));
            return false;
        }
    }

    const bool has_positive_filter = std::any_of(filters.begin(), filters.end(), [](const std::string& pattern) {
        return !pattern.empty() && pattern[0] != '!';
    });

    std::error_code ec;
    std::filesystem::path root = ComputeGlobRoot(dir_pattern);
    if (!root.is_absolute()) {
        root = std::filesystem::absolute(root, ec);
        if (ec) {
            isolate->ThrowException(v8::Exception::Error(
                v8::String::NewFromUtf8Literal(isolate, "Glob failed to resolve root path")));
            return false;
        }
    }

    v8::Local<v8::Array> result = v8::Array::New(isolate);
    uint32_t index = 0;

    for (std::filesystem::recursive_directory_iterator it(root, ec), end; !ec && it != end; it.increment(ec)) {
        const std::filesystem::directory_entry& entry = *it;
        if (!entry.is_regular_file(ec) || ec) {
            continue;
        }

        const std::string absolute = NormalizePathPattern(entry.path().string());
        std::string relative;
        std::error_code rel_ec;
        relative = NormalizePathPattern(std::filesystem::relative(entry.path(), std::filesystem::current_path(), rel_ec).string());
        if (rel_ec) {
            relative = NormalizePathPattern(entry.path().filename().string());
        }
        const std::string filename = NormalizePathPattern(entry.path().filename().string());

        if (!MatchAnyPattern(dir_pattern, relative, absolute, filename)) {
            continue;
        }

        bool include = !has_positive_filter;
        for (const std::string& raw_filter : filters) {
            if (raw_filter.empty()) {
                continue;
            }

            const bool exclude = raw_filter[0] == '!';
            const std::string filter_pattern = exclude ? raw_filter.substr(1) : raw_filter;
            if (filter_pattern.empty()) {
                continue;
            }

            if (!MatchAnyPattern(filter_pattern, relative, absolute, filename)) {
                continue;
            }

            if (exclude) {
                include = false;
                break;
            }
            include = true;
        }

        if (!include) {
            continue;
        }

        (void)result
            ->Set(context,
                  index++,
                  v8::String::NewFromUtf8(isolate, relative.c_str()).ToLocalChecked())
            .FromMaybe(false);
    }

    if (ec) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Glob scan failed")));
        return false;
    }

    *out_result = result;
    return true;
}

void GlobCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsObject()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Glob expects options object")));
        return;
    }

    v8::Local<v8::Array> result;
    if (!BuildGlobResult(isolate, context, args[0].As<v8::Object>(), &result)) {
        return;
    }

    args.GetReturnValue().Set(result);
}

void GlobConstructorCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (!args.IsConstructCall()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Use new Glob({ dir, filter })")));
        return;
    }

    if (args.Length() < 1 || !args[0]->IsObject()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Glob expects options object")));
        return;
    }

    v8::Local<v8::Array> result;
    if (!BuildGlobResult(isolate, context, args[0].As<v8::Object>(), &result)) {
        return;
    }

    args.GetReturnValue().Set(result);
}

void CreateDirectoryCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "CreateDirectory expects a string path")));
        return;
    }

    v8::String::Utf8Value path_utf8(isolate, args[0]);
    if (*path_utf8 == nullptr) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Invalid UTF-8 path")));
        return;
    }

    std::string absolute = FileSystem::CreateDirectory(*path_utf8);
    if (absolute.empty()) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to create directory")));
        return;
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, absolute.c_str()).ToLocalChecked());
}

void ExistsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        ThrowPathTypeError(isolate, "Exists expects a string path");
        return;
    }

    std::error_code ec;
    const bool exists = std::filesystem::exists(ValueToString(isolate, args[0]), ec);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, !ec && exists));
}

void IsDirectoryCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        ThrowPathTypeError(isolate, "IsDirectory expects a string path");
        return;
    }

    std::error_code ec;
    const bool ok = std::filesystem::is_directory(ValueToString(isolate, args[0]), ec);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, !ec && ok));
}

void IsFileCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        ThrowPathTypeError(isolate, "IsFile expects a string path");
        return;
    }

    std::error_code ec;
    const bool ok = std::filesystem::is_regular_file(ValueToString(isolate, args[0]), ec);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, !ec && ok));
}

void ReadTextCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        ThrowPathTypeError(isolate, "ReadText expects a string path");
        return;
    }

    const std::string path = ValueToString(isolate, args[0]);
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec || !std::filesystem::is_regular_file(path, ec) || ec) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to open file for reading")));
        return;
    }

    const std::string content = ToolTo::ReadTextFile(path);
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(isolate, content.data(), v8::NewStringType::kNormal, content.size())
            .ToLocalChecked());
}

void WriteTextImpl(const v8::FunctionCallbackInfo<v8::Value>& args, bool append) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString()) {
        ThrowPathTypeError(isolate, append ? "AppendText expects path and text" : "WriteText expects path and text");
        return;
    }

    const std::string path = ValueToString(isolate, args[0]);
    const std::string content = ValueToString(isolate, args[1]);

    std::string write_error;
    if (!ToolTo::WriteTextFile(path, content, append, &write_error)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to open file for writing")));
        return;
    }

    args.GetReturnValue().Set(v8::Boolean::New(isolate, true));
}

void WriteTextCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    WriteTextImpl(args, false);
}

void AppendTextCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    WriteTextImpl(args, true);
}

void RemovePathCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsString()) {
        ThrowPathTypeError(isolate, "Remove expects a string path");
        return;
    }

    std::error_code ec;
    const bool removed = std::filesystem::remove_all(ValueToString(isolate, args[0]), ec) > 0;
    args.GetReturnValue().Set(v8::Boolean::New(isolate, !ec && removed));
}

void ListDirectoryCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    std::string path = ".";
    if (args.Length() > 0) {
        if (!args[0]->IsString()) {
            ThrowPathTypeError(isolate, "ListDirectory expects optional string path");
            return;
        }
        path = ValueToString(isolate, args[0]);
    }

    std::error_code ec;
    v8::Local<v8::Array> result = v8::Array::New(isolate);
    uint32_t index = 0;
    for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
        if (ec) {
            break;
        }
        const std::string name = entry.path().filename().string();
        (void)result->Set(
            context,
            index++,
            v8::String::NewFromUtf8(isolate, name.c_str()).ToLocalChecked()).FromMaybe(false);
    }

    args.GetReturnValue().Set(result);
}

void CurrentPathCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    std::error_code ec;
    const std::string cwd = std::filesystem::current_path(ec).string();
    if (ec) {
        args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(args.GetIsolate(), ""));
        return;
    }

    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), cwd.c_str()).ToLocalChecked());
}

}  // namespace

namespace modules {

bool RegisterFileSystemModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> module = v8::Object::New(isolate);
    v8::Local<v8::Function> create_directory_fn =
        v8::Function::New(context, CreateDirectoryCallback).ToLocalChecked();

    bool ok = module
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "CreateDirectory"),
                        create_directory_fn)
                  .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Exists"),
                     v8::Function::New(context, ExistsCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "IsDirectory"),
                     v8::Function::New(context, IsDirectoryCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "IsFile"),
                     v8::Function::New(context, IsFileCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "ReadText"),
                     v8::Function::New(context, ReadTextCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "WriteText"),
                     v8::Function::New(context, WriteTextCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "AppendText"),
                     v8::Function::New(context, AppendTextCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Remove"),
                     v8::Function::New(context, RemovePathCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "ListDirectory"),
                     v8::Function::New(context, ListDirectoryCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "CurrentPath"),
                     v8::Function::New(context, CurrentPathCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Glob"),
                     v8::Function::New(context, GlobCallback).ToLocalChecked())
                 .FromMaybe(false);
    if (!ok) {
        return false;
    }

    v8::Local<v8::Function> glob_ctor =
        v8::Function::New(context, GlobConstructorCallback).ToLocalChecked();
    bool global_ok = context->Global()
                         ->Set(context, v8::String::NewFromUtf8Literal(isolate, "FileSystem"), module)
                         .FromMaybe(false);
    global_ok = global_ok && context->Global()
                                 ->Set(context,
                                       v8::String::NewFromUtf8Literal(isolate, "Glob"),
                                       glob_ctor)
                                 .FromMaybe(false);
    return global_ok;
}

}  // namespace modules
