#include "modules/runtime_live_module.h"

#include "runtime_live.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return *utf8;
}

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return std::string();
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::filesystem::path ResolveListRoot(const std::string& root) {
    std::filesystem::path root_path(root);
    if (std::filesystem::exists(root_path)) {
        return root_path;
    }

    std::filesystem::path example_root = std::filesystem::path("example") / root;
    if (std::filesystem::exists(example_root)) {
        return example_root;
    }

    return root_path;
}

void EmitEvent(v8::Isolate* isolate,
               v8::Local<v8::Context> context,
               v8::Local<v8::Function> on_event,
               const char* type,
               const std::string& message) {
    if (on_event.IsEmpty()) {
        return;
    }

    v8::Local<v8::Object> event = v8::Object::New(isolate);
    (void)event
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "type"),
              v8::String::NewFromUtf8(isolate, type).ToLocalChecked())
        .FromMaybe(false);
    (void)event
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "message"),
              v8::String::NewFromUtf8(isolate, message.c_str()).ToLocalChecked())
        .FromMaybe(false);

    v8::Local<v8::Value> argv[] = {event};
    (void)on_event->Call(context, context->Global(), 1, argv);
}

bool BuildEntriesFromList(v8::Isolate* isolate,
                          v8::Local<v8::Context> context,
                          v8::Local<v8::Object> list,
                          std::vector<RuntimeLive::InterfaceCompiler::SourceFile>* out_entries,
                          bool* out_uses_cpp,
                          std::string* error_message) {
    struct SourceChunk {
        std::string file_name;
        std::string content;
        bool is_header;
    };

    v8::Local<v8::Value> root_value;
    v8::Local<v8::Value> entries_value;
    if (!list->Get(context, v8::String::NewFromUtf8Literal(isolate, "__root")).ToLocal(&root_value) ||
        !root_value->IsString() ||
        !list->Get(context, v8::String::NewFromUtf8Literal(isolate, "__entries")).ToLocal(&entries_value) ||
        !entries_value->IsArray()) {
        *error_message = "Container list is corrupted";
        return false;
    }

    const std::filesystem::path root = ResolveListRoot(ValueToString(isolate, root_value));
    const v8::Local<v8::Array> entries = entries_value.As<v8::Array>();
    std::vector<SourceChunk> source_chunks;
    bool uses_cpp = false;

    for (uint32_t i = 0; i < entries->Length(); ++i) {
        v8::Local<v8::Value> entry_value;
        if (!entries->Get(context, i).ToLocal(&entry_value) || !entry_value->IsObject()) {
            *error_message = "Container entry must be an object";
            return false;
        }

        v8::Local<v8::Object> entry = entry_value.As<v8::Object>();

        v8::Local<v8::Value> on_start_candidate;
        if (entry->Get(context, v8::String::NewFromUtf8Literal(isolate, "onStart")).ToLocal(&on_start_candidate) &&
            on_start_candidate->IsFunction()) {
            v8::Local<v8::Function> on_start = on_start_candidate.As<v8::Function>();
            v8::Local<v8::Value> argv[] = {entry};
            (void)on_start->Call(context, context->Global(), 1, argv);
        }

        v8::Local<v8::Value> path_value;
        if (!entry->Get(context, v8::String::NewFromUtf8Literal(isolate, "path")).ToLocal(&path_value) ||
            !path_value->IsString()) {
            *error_message = "Container entry.path must be a string";
            return false;
        }

        v8::Local<v8::Value> header_value;
        if (!entry->Get(context, v8::String::NewFromUtf8Literal(isolate, "header")).ToLocal(&header_value)) {
            header_value = v8::Undefined(isolate);
        }

        const std::filesystem::path source_path = root / ValueToString(isolate, path_value);
        const std::string extension = source_path.extension().string();
        if (extension == ".cc" || extension == ".cpp" || extension == ".cxx") {
            uses_cpp = true;
        }

        if (header_value->IsString()) {
            const std::filesystem::path header_path = root / ValueToString(isolate, header_value);
            const std::string header_source = ReadTextFile(header_path);
            if (header_source.empty()) {
                *error_message = "Unable to read header: " + header_path.string();
                return false;
            }
            source_chunks.push_back({ValueToString(isolate, header_value), header_source, true});
        }

        const std::string source = ReadTextFile(source_path);
        if (source.empty()) {
            *error_message = "Unable to read source: " + source_path.string();
            return false;
        }
        source_chunks.push_back({source_path.filename().string(), source, false});

        v8::Local<v8::Value> on_end_candidate;
        if (entry->Get(context, v8::String::NewFromUtf8Literal(isolate, "onEnd")).ToLocal(&on_end_candidate) &&
            on_end_candidate->IsFunction()) {
            v8::Local<v8::Function> on_end = on_end_candidate.As<v8::Function>();
            std::string message = "Loaded " + source_path.filename().string();
            v8::Local<v8::Value> argv[] = {
                v8::String::NewFromUtf8(isolate, message.c_str()).ToLocalChecked()};
            (void)on_end->Call(context, context->Global(), 1, argv);
        }
    }

    out_entries->clear();
    for (const auto& chunk : source_chunks) {
        out_entries->push_back({chunk.file_name, chunk.content, chunk.is_header});
    }

    *out_uses_cpp = uses_cpp;
    return true;
}

v8::Local<v8::Array> BuildEntriesArray(v8::Isolate* isolate,
                                       v8::Local<v8::Context> context,
                                       const std::vector<RuntimeLive::InterfaceCompiler::SourceFile>& entries) {
    v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(entries.size()));
    for (uint32_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        v8::Local<v8::Object> obj = v8::Object::New(isolate);
        (void)obj
            ->Set(context,
                  v8::String::NewFromUtf8Literal(isolate, "path"),
                  v8::String::NewFromUtf8(isolate, entry.path.c_str()).ToLocalChecked())
            .FromMaybe(false);
        (void)obj
            ->Set(context,
                  v8::String::NewFromUtf8Literal(isolate, "content"),
                  v8::String::NewFromUtf8(isolate, entry.content.c_str()).ToLocalChecked())
            .FromMaybe(false);
        (void)obj
            ->Set(context,
                  v8::String::NewFromUtf8Literal(isolate, "is_header"),
                  v8::Boolean::New(isolate, entry.is_header))
            .FromMaybe(false);
        (void)array->Set(context, i, obj).FromMaybe(false);
    }
    return array;
}

void ReadEntriesArray(v8::Isolate* isolate,
                      v8::Local<v8::Context> context,
                      v8::Local<v8::Object> self,
                      std::vector<RuntimeLive::InterfaceCompiler::SourceFile>* out_entries) {
    out_entries->clear();

    v8::Local<v8::Value> entries_value;
    if (!self->Get(context, v8::String::NewFromUtf8Literal(isolate, "__entries")).ToLocal(&entries_value) ||
        !entries_value->IsArray()) {
        return;
    }

    v8::Local<v8::Array> entries = entries_value.As<v8::Array>();
    for (uint32_t i = 0; i < entries->Length(); ++i) {
        v8::Local<v8::Value> entry_value;
        if (!entries->Get(context, i).ToLocal(&entry_value) || !entry_value->IsObject()) {
            continue;
        }

        v8::Local<v8::Object> entry_obj = entry_value.As<v8::Object>();
        v8::Local<v8::Value> path_value;
        v8::Local<v8::Value> content_value;
        v8::Local<v8::Value> is_header_value;

        if (!entry_obj->Get(context, v8::String::NewFromUtf8Literal(isolate, "path")).ToLocal(&path_value) ||
            !path_value->IsString() ||
            !entry_obj->Get(context, v8::String::NewFromUtf8Literal(isolate, "content")).ToLocal(&content_value) ||
            !content_value->IsString()) {
            continue;
        }

        bool is_header = false;
        if (entry_obj->Get(context, v8::String::NewFromUtf8Literal(isolate, "is_header")).ToLocal(&is_header_value) &&
            is_header_value->IsBoolean()) {
            is_header = is_header_value.As<v8::Boolean>()->Value();
        }

        out_entries->push_back({
            ValueToString(isolate, path_value),
            ValueToString(isolate, content_value),
            is_header});
    }
}

bool ExtractCompilerState(v8::Isolate* isolate,
                          v8::Local<v8::Context> context,
                          v8::Local<v8::Object> self,
                          std::string* cache_dir,
                          std::string* provider,
                          std::string* source,
                          std::string* language,
                          std::string* compile_flags,
                          std::string* link_flags) {
    v8::Local<v8::Value> cache_value;
    v8::Local<v8::Value> provider_value;
    v8::Local<v8::Value> source_value;
    v8::Local<v8::Value> language_value;
    v8::Local<v8::Value> compile_flags_value;
    v8::Local<v8::Value> link_flags_value;

    if (!self->Get(context, v8::String::NewFromUtf8Literal(isolate, "__cache_dir")).ToLocal(&cache_value) ||
        !cache_value->IsString() ||
        !self->Get(context, v8::String::NewFromUtf8Literal(isolate, "__provider")).ToLocal(&provider_value) ||
        !provider_value->IsString() ||
        !self->Get(context, v8::String::NewFromUtf8Literal(isolate, "__source")).ToLocal(&source_value) ||
        !source_value->IsString() ||
        !self->Get(context, v8::String::NewFromUtf8Literal(isolate, "__language")).ToLocal(&language_value) ||
        !language_value->IsString() ||
        !self->Get(context, v8::String::NewFromUtf8Literal(isolate, "__compile_flags")).ToLocal(&compile_flags_value) ||
        !compile_flags_value->IsString() ||
        !self->Get(context, v8::String::NewFromUtf8Literal(isolate, "__link_flags")).ToLocal(&link_flags_value) ||
        !link_flags_value->IsString()) {
        return false;
    }

    *cache_dir = ValueToString(isolate, cache_value);
    *provider = ValueToString(isolate, provider_value);
    *source = ValueToString(isolate, source_value);
    *language = ValueToString(isolate, language_value);
    *compile_flags = ValueToString(isolate, compile_flags_value);
    *link_flags = ValueToString(isolate, link_flags_value);
    return true;
}

bool RunCompiler(v8::Isolate* isolate,
                 v8::Local<v8::Context> context,
                 v8::Local<v8::Object> self,
                 v8::Local<v8::Function> raw_out,
                 v8::Local<v8::Function> raw_err,
                 v8::Local<v8::Function> on_event,
                 const std::string& output_copy_path,
                 bool emit_debug,
                 std::string* error_message) {
    std::string cache_dir;
    std::string provider;
    std::string source;
    std::string language;
    std::string compile_flags;
    std::string link_flags;
    if (!ExtractCompilerState(isolate,
                              context,
                              self,
                              &cache_dir,
                              &provider,
                              &source,
                              &language,
                              &compile_flags,
                              &link_flags)) {
        *error_message = "Interface compiler is corrupted";
        return false;
    }

    std::vector<RuntimeLive::InterfaceCompiler::SourceFile> file_entries;
    ReadEntriesArray(isolate, context, self, &file_entries);

    if (emit_debug) {
        EmitEvent(isolate, context, on_event, "compile_start", "Starting compile and run");
    }

    RuntimeLive::InterfaceCompiler compiler(cache_dir, provider, language, compile_flags, link_flags);
    compiler.ClearEntryFiles();
    if (file_entries.empty()) {
        compiler.AddEntryRaw(source);
    } else {
        for (const auto& file_entry : file_entries) {
            compiler.AddEntryFile(file_entry);
        }
    }

    RuntimeLive::RuntimeCallbacks native_callbacks;
    native_callbacks.raw_out = [&](const std::string& text) {
        if (!raw_out.IsEmpty()) {
            v8::Local<v8::Value> argv[] = {
                v8::String::NewFromUtf8(isolate, text.c_str()).ToLocalChecked()};
            (void)raw_out->Call(context, context->Global(), 1, argv);
        }
        if (emit_debug) {
            EmitEvent(isolate, context, on_event, "stdout", text);
        }
    };
    native_callbacks.raw_err = [&](const std::string& text) {
        if (!raw_err.IsEmpty()) {
            v8::Local<v8::Value> argv[] = {
                v8::String::NewFromUtf8(isolate, text.c_str()).ToLocalChecked()};
            (void)raw_err->Call(context, context->Global(), 1, argv);
        }
        if (emit_debug && !text.empty()) {
            EmitEvent(isolate, context, on_event, "stderr", text);
        }
    };

    const bool result = compiler.Runtime(native_callbacks);

    if (!output_copy_path.empty() && result) {
        std::error_code ec;
        std::filesystem::copy_file(std::filesystem::path(cache_dir) / "entry.out",
                                   output_copy_path,
                                   std::filesystem::copy_options::overwrite_existing,
                                   ec);
        if (ec) {
            *error_message = "Failed to write output: " + output_copy_path;
            return false;
        }
    }

    if (emit_debug) {
        EmitEvent(isolate,
                  context,
                  on_event,
                  result ? "success" : "failed",
                  result ? "Compilation and run completed" : "Compilation or run failed");
    }

    return result;
}

void AddEntryRawCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "AddEntryRaw expects a source string")));
        return;
    }

    v8::Local<v8::Object> self = args.This();
    bool ok = self
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__source"),
                        args[0])
                  .FromMaybe(false);
        ok = ok && self
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "__entries"),
                     v8::Array::New(isolate))
                 .FromMaybe(false);
    if (!ok) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Unable to persist source")));
    }
}

void AddEntryFromListCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsObject()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "AddEntryFromList expects a list object")));
        return;
    }

    std::vector<RuntimeLive::InterfaceCompiler::SourceFile> entries;
    bool uses_cpp = false;
    std::string error_message;
    if (!BuildEntriesFromList(isolate,
                              context,
                              args[0].As<v8::Object>(),
                              &entries,
                              &uses_cpp,
                              &error_message)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, error_message.c_str()).ToLocalChecked()));
        return;
    }

    v8::Local<v8::Object> self = args.This();
    bool ok = self
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__source"),
                    v8::String::NewFromUtf8Literal(isolate, ""))
                .FromMaybe(false);
        ok = ok && self
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "__entries"),
                     BuildEntriesArray(isolate, context, entries))
                  .FromMaybe(false);
    if (ok && uses_cpp) {
        v8::Local<v8::Value> language_value;
        if (self->Get(context, v8::String::NewFromUtf8Literal(isolate, "__language")).ToLocal(&language_value) &&
            language_value->IsString() &&
            ValueToString(isolate, language_value) == "c") {
            ok = self
                     ->Set(context,
                           v8::String::NewFromUtf8Literal(isolate, "__language"),
                           v8::String::NewFromUtf8Literal(isolate, "cpp"))
                     .FromMaybe(false);
        }
    }
    if (!ok) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Unable to persist source")));
    }
}

void RuntimeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::Local<v8::Function> raw_out;
    v8::Local<v8::Function> raw_err;
    if (args.Length() > 0 && args[0]->IsObject()) {
        v8::Local<v8::Object> callbacks = args[0].As<v8::Object>();
        v8::Local<v8::Value> out_candidate;
        if (callbacks->Get(context, v8::String::NewFromUtf8Literal(isolate, "raw_out")).ToLocal(&out_candidate) &&
            out_candidate->IsFunction()) {
            raw_out = out_candidate.As<v8::Function>();
        }

        v8::Local<v8::Value> err_candidate;
        if (callbacks->Get(context, v8::String::NewFromUtf8Literal(isolate, "raw_err")).ToLocal(&err_candidate) &&
            err_candidate->IsFunction()) {
            raw_err = err_candidate.As<v8::Function>();
        }
    }

    std::string error_message;
    bool result = RunCompiler(isolate,
                              context,
                              args.This(),
                              raw_out,
                              raw_err,
                              v8::Local<v8::Function>(),
                              std::string(),
                              false,
                              &error_message);
    if (!result && !error_message.empty()) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, error_message.c_str()).ToLocalChecked()));
        return;
    }

    args.GetReturnValue().Set(v8::Boolean::New(isolate, result));
}

void RuntimeToCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::Local<v8::Function> raw_out;
    v8::Local<v8::Function> raw_err;
    v8::Local<v8::Function> on_event;
    std::string output_path;
    bool debug = false;

    if (args.Length() > 0 && args[0]->IsObject()) {
        v8::Local<v8::Object> options = args[0].As<v8::Object>();

        v8::Local<v8::Value> output_value;
        if (options->Get(context, v8::String::NewFromUtf8Literal(isolate, "output")).ToLocal(&output_value) &&
            output_value->IsString()) {
            output_path = ValueToString(isolate, output_value);
        }

        v8::Local<v8::Value> debug_value;
        if (options->Get(context, v8::String::NewFromUtf8Literal(isolate, "debug")).ToLocal(&debug_value) &&
            debug_value->IsBoolean()) {
            debug = debug_value.As<v8::Boolean>()->Value();
        }

        v8::Local<v8::Value> event_value;
        if (options->Get(context, v8::String::NewFromUtf8Literal(isolate, "onEvent")).ToLocal(&event_value) &&
            event_value->IsFunction()) {
            on_event = event_value.As<v8::Function>();
        }

        v8::Local<v8::Value> out_candidate;
        if (options->Get(context, v8::String::NewFromUtf8Literal(isolate, "raw_out")).ToLocal(&out_candidate) &&
            out_candidate->IsFunction()) {
            raw_out = out_candidate.As<v8::Function>();
        }

        v8::Local<v8::Value> err_candidate;
        if (options->Get(context, v8::String::NewFromUtf8Literal(isolate, "raw_err")).ToLocal(&err_candidate) &&
            err_candidate->IsFunction()) {
            raw_err = err_candidate.As<v8::Function>();
        }
    }

    std::string error_message;
    bool result = RunCompiler(isolate,
                              context,
                              args.This(),
                              raw_out,
                              raw_err,
                              on_event,
                              output_path,
                              debug,
                              &error_message);
    if (!result && !error_message.empty()) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, error_message.c_str()).ToLocalChecked()));
        return;
    }

    args.GetReturnValue().Set(v8::Boolean::New(isolate, result));
}

bool SetStringField(v8::Isolate* isolate,
                    v8::Local<v8::Context> context,
                    v8::Local<v8::Object> self,
                    const char* key,
                    v8::Local<v8::Value> value) {
    if (!value->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8(isolate, key).ToLocalChecked()));
        return false;
    }
    return self
        ->Set(context,
              v8::String::NewFromUtf8(isolate, key).ToLocalChecked(),
              value)
        .FromMaybe(false);
}

void SetProviderCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "SetProvider expects string")));
        return;
    }

    bool ok = args.This()
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__provider"),
                        args[0])
                  .FromMaybe(false);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void SetLanguageCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "SetLanguage expects string")));
        return;
    }

    bool ok = args.This()
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__language"),
                        args[0])
                  .FromMaybe(false);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void SetCompileFlagsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "SetCompileFlags expects string")));
        return;
    }

    bool ok = args.This()
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__compile_flags"),
                        args[0])
                  .FromMaybe(false);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void SetLinkFlagsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "SetLinkFlags expects string")));
        return;
    }

    bool ok = args.This()
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__link_flags"),
                        args[0])
                  .FromMaybe(false);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void SetCacheDirCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "SetCacheDir expects string")));
        return;
    }

    bool ok = args.This()
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__cache_dir"),
                        args[0])
                  .FromMaybe(false);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

bool AppendFlags(v8::Isolate* isolate,
                 v8::Local<v8::Context> context,
                 v8::Local<v8::Object> self,
                 const char* field,
                 const std::string& addition) {
    if (addition.empty()) {
        return true;
    }

    v8::Local<v8::Value> current_value;
    if (!self->Get(context, v8::String::NewFromUtf8(isolate, field).ToLocalChecked()).ToLocal(&current_value) ||
        !current_value->IsString()) {
        return false;
    }

    std::string flags = ValueToString(isolate, current_value);
    if (!flags.empty()) {
        flags += " ";
    }
    flags += addition;

    return self
        ->Set(context,
              v8::String::NewFromUtf8(isolate, field).ToLocalChecked(),
              v8::String::NewFromUtf8(isolate, flags.c_str()).ToLocalChecked())
        .FromMaybe(false);
}

void AddCompileFlagsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "AddCompileFlags expects string")));
        return;
    }

    bool ok = AppendFlags(isolate, context, args.This(), "__compile_flags", ValueToString(isolate, args[0]));
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void AddLinkFlagsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "AddLinkFlags expects string")));
        return;
    }

    bool ok = AppendFlags(isolate, context, args.This(), "__link_flags", ValueToString(isolate, args[0]));
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void AddDefineCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "AddDefine expects symbol")));
        return;
    }

    const std::string define = "-D" + ValueToString(isolate, args[0]);
    bool ok = AppendFlags(isolate, context, args.This(), "__compile_flags", define);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void AddIncludeDirCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "AddIncludeDir expects path")));
        return;
    }

    const std::string include_flag = "-I" + ValueToString(isolate, args[0]);
    bool ok = AppendFlags(isolate, context, args.This(), "__compile_flags", include_flag);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void AddLibraryDirCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "AddLibraryDir expects path")));
        return;
    }

    const std::string lib_dir_flag = "-L" + ValueToString(isolate, args[0]);
    bool ok = AppendFlags(isolate, context, args.This(), "__link_flags", lib_dir_flag);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void AddLibraryCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "AddLibrary expects library name")));
        return;
    }

    const std::string lib_flag = "-l" + ValueToString(isolate, args[0]);
    bool ok = AppendFlags(isolate, context, args.This(), "__link_flags", lib_flag);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void AddEntryFileCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsObject()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "AddEntryFile expects object")));
        return;
    }

    v8::Local<v8::Object> file_obj = args[0].As<v8::Object>();
    v8::Local<v8::Value> path_value;
    v8::Local<v8::Value> content_value;
    if (!file_obj->Get(context, v8::String::NewFromUtf8Literal(isolate, "path")).ToLocal(&path_value) ||
        !path_value->IsString() ||
        !file_obj->Get(context, v8::String::NewFromUtf8Literal(isolate, "content")).ToLocal(&content_value) ||
        !content_value->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "AddEntryFile expects path/content strings")));
        return;
    }

    v8::Local<v8::Value> is_header_value;
    bool is_header = false;
    if (file_obj->Get(context, v8::String::NewFromUtf8Literal(isolate, "is_header")).ToLocal(&is_header_value) &&
        is_header_value->IsBoolean()) {
        is_header = is_header_value.As<v8::Boolean>()->Value();
    }

    v8::Local<v8::Value> entries_value;
    if (!args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "__entries")).ToLocal(&entries_value) ||
        !entries_value->IsArray()) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Compiler entries are corrupted")));
        return;
    }

    v8::Local<v8::Array> entries = entries_value.As<v8::Array>();
    v8::Local<v8::Object> entry = v8::Object::New(isolate);
    bool ok = entry
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "path"),
                        path_value)
                  .FromMaybe(false);
    ok = ok && entry
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "content"),
                         content_value)
                   .FromMaybe(false);
    ok = ok && entry
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "is_header"),
                         v8::Boolean::New(isolate, is_header))
                   .FromMaybe(false);
    ok = ok && entries->Set(context, entries->Length(), entry).FromMaybe(false);

    if (ok) {
        (void)args.This()
            ->Set(context,
                  v8::String::NewFromUtf8Literal(isolate, "__source"),
                  v8::String::NewFromUtf8Literal(isolate, ""))
            .FromMaybe(false);
    }

    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void ClearEntriesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    bool ok = args.This()
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__entries"),
                        v8::Array::New(isolate))
                  .FromMaybe(false);
    ok = ok && args.This()
                 ->Set(context,
                       v8::String::NewFromUtf8Literal(isolate, "__source"),
                       v8::String::NewFromUtf8Literal(isolate, ""))
                 .FromMaybe(false);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void GetEntriesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::Local<v8::Value> entries;
    if (!args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "__entries")).ToLocal(&entries)) {
        args.GetReturnValue().Set(v8::Array::New(isolate));
        return;
    }
    args.GetReturnValue().Set(entries);
}

void GetConfigCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::Local<v8::Object> config = v8::Object::New(isolate);
    const char* keys[] = {
        "__cache_dir", "__provider", "__language", "__compile_flags", "__link_flags"};
    const char* out_keys[] = {
        "cache_dir", "provider", "language", "compile_flags", "link_flags"};

    for (int i = 0; i < 5; ++i) {
        v8::Local<v8::Value> value;
        if (args.This()->Get(context, v8::String::NewFromUtf8(isolate, keys[i]).ToLocalChecked()).ToLocal(&value)) {
            (void)config
                ->Set(context,
                      v8::String::NewFromUtf8(isolate, out_keys[i]).ToLocalChecked(),
                      value)
                .FromMaybe(false);
        }
    }

    args.GetReturnValue().Set(config);
}

void BuildPlanCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    std::string cache_dir;
    std::string provider;
    std::string source;
    std::string language;
    std::string compile_flags;
    std::string link_flags;
    if (!ExtractCompilerState(isolate,
                              context,
                              args.This(),
                              &cache_dir,
                              &provider,
                              &source,
                              &language,
                              &compile_flags,
                              &link_flags)) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Interface compiler is corrupted")));
        return;
    }

    std::vector<RuntimeLive::InterfaceCompiler::SourceFile> entries;
    ReadEntriesArray(isolate, context, args.This(), &entries);

    v8::Local<v8::Object> plan = v8::Object::New(isolate);
    (void)plan
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "cache_dir"),
              v8::String::NewFromUtf8(isolate, cache_dir.c_str()).ToLocalChecked())
        .FromMaybe(false);
    (void)plan
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "provider"),
              v8::String::NewFromUtf8(isolate, provider.c_str()).ToLocalChecked())
        .FromMaybe(false);
    (void)plan
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "language"),
              v8::String::NewFromUtf8(isolate, language.c_str()).ToLocalChecked())
        .FromMaybe(false);
    (void)plan
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "compile_flags"),
              v8::String::NewFromUtf8(isolate, compile_flags.c_str()).ToLocalChecked())
        .FromMaybe(false);
    (void)plan
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "link_flags"),
              v8::String::NewFromUtf8(isolate, link_flags.c_str()).ToLocalChecked())
        .FromMaybe(false);
    (void)plan
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "entry_count"),
              v8::Integer::New(isolate, static_cast<int>(entries.size())))
        .FromMaybe(false);
    (void)plan
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "uses_entry_raw"),
              v8::Boolean::New(isolate, entries.empty() && !source.empty()))
        .FromMaybe(false);

    args.GetReturnValue().Set(plan);
}

void CreateInterfaceCompilerCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsObject()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "CreateInterfaceCompiler expects options object")));
        return;
    }

    v8::Local<v8::Object> options = args[0].As<v8::Object>();
    v8::Local<v8::Value> cache_dir_value;
    if (!options->Get(context, v8::String::NewFromUtf8Literal(isolate, "cache_dir")).ToLocal(&cache_dir_value) ||
        !cache_dir_value->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "cache_dir must be a string")));
        return;
    }

    v8::Local<v8::Value> provider_value;
    if (!options->Get(context, v8::String::NewFromUtf8Literal(isolate, "provider")).ToLocal(&provider_value)) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "provider option could not be read")));
        return;
    }
    if (!provider_value->IsString()) {
        provider_value = v8::String::NewFromUtf8Literal(isolate, "");
    }

    v8::Local<v8::Value> language_value;
    if (!options->Get(context, v8::String::NewFromUtf8Literal(isolate, "language")).ToLocal(&language_value) ||
        !language_value->IsString()) {
        language_value = v8::String::NewFromUtf8Literal(isolate, "c");
    }

    v8::Local<v8::Value> compile_flags_value;
    if (!options->Get(context, v8::String::NewFromUtf8Literal(isolate, "compile_flags")).ToLocal(&compile_flags_value) ||
        !compile_flags_value->IsString()) {
        compile_flags_value = v8::String::NewFromUtf8Literal(isolate, "");
    }

    v8::Local<v8::Value> link_flags_value;
    if (!options->Get(context, v8::String::NewFromUtf8Literal(isolate, "link_flags")).ToLocal(&link_flags_value) ||
        !link_flags_value->IsString()) {
        link_flags_value = v8::String::NewFromUtf8Literal(isolate, "");
    }

    v8::Local<v8::Object> compiler = v8::Object::New(isolate);
    bool ok = compiler
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__cache_dir"),
                        cache_dir_value)
                  .FromMaybe(false);
    ok = ok && compiler
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "__source"),
                         v8::String::NewFromUtf8Literal(isolate, ""))
                   .FromMaybe(false);
        ok = ok && compiler
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "__entries"),
                     v8::Array::New(isolate))
                 .FromMaybe(false);
    ok = ok && compiler
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "__provider"),
                     provider_value)
                 .FromMaybe(false);
    ok = ok && compiler
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "__language"),
                     language_value)
                 .FromMaybe(false);
    ok = ok && compiler
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "__compile_flags"),
                     compile_flags_value)
                 .FromMaybe(false);
    ok = ok && compiler
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "__link_flags"),
                     link_flags_value)
                 .FromMaybe(false);
    ok = ok && compiler
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "AddEntryRaw"),
                         v8::Function::New(context, AddEntryRawCallback).ToLocalChecked())
                   .FromMaybe(false);
        ok = ok && compiler
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "AddEntryFromList"),
                     v8::Function::New(context, AddEntryFromListCallback).ToLocalChecked())
                 .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "AddEntryFile"),
                         v8::Function::New(context, AddEntryFileCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "ClearEntries"),
                         v8::Function::New(context, ClearEntriesCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "GetEntries"),
                         v8::Function::New(context, GetEntriesCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "SetProvider"),
                         v8::Function::New(context, SetProviderCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "SetLanguage"),
                         v8::Function::New(context, SetLanguageCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "SetCacheDir"),
                         v8::Function::New(context, SetCacheDirCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "SetCompileFlags"),
                         v8::Function::New(context, SetCompileFlagsCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "SetLinkFlags"),
                         v8::Function::New(context, SetLinkFlagsCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "AddCompileFlags"),
                         v8::Function::New(context, AddCompileFlagsCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "AddLinkFlags"),
                         v8::Function::New(context, AddLinkFlagsCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "AddDefine"),
                         v8::Function::New(context, AddDefineCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "AddIncludeDir"),
                         v8::Function::New(context, AddIncludeDirCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "AddLibraryDir"),
                         v8::Function::New(context, AddLibraryDirCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "AddLibrary"),
                         v8::Function::New(context, AddLibraryCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "GetConfig"),
                         v8::Function::New(context, GetConfigCallback).ToLocalChecked())
                     .FromMaybe(false);
            ok = ok && compiler
                     ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "BuildPlan"),
                         v8::Function::New(context, BuildPlanCallback).ToLocalChecked())
                     .FromMaybe(false);
    ok = ok && compiler
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "Runtime"),
                         v8::Function::New(context, RuntimeCallback).ToLocalChecked())
                   .FromMaybe(false);
        ok = ok && compiler
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "RuntimeTo"),
                     v8::Function::New(context, RuntimeToCallback).ToLocalChecked())
                 .FromMaybe(false);

    if (!ok) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to create compiler interface")));
        return;
    }

    args.GetReturnValue().Set(compiler);
}

}  // namespace

namespace modules {

bool RegisterRuntimeLiveModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> module = v8::Object::New(isolate);
    bool ok = module
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "CreateInterfaceCompiler"),
                        v8::Function::New(context, CreateInterfaceCompilerCallback).ToLocalChecked())
                  .FromMaybe(false);
    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "RuntimeLive"), module)
        .FromMaybe(false);
}

}  // namespace modules
