#include "modules/container_module.h"

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <filesystem>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

struct XmlFileEntry {
    std::string path;
    bool is_header = false;
};

std::string ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return *utf8;
}

std::string XmlCharToString(const xmlChar* value) {
    if (value == nullptr) {
        return std::string();
    }
    return reinterpret_cast<const char*>(value);
}

void WalkXml(xmlNode* node,
             std::vector<std::string>* directories,
             std::vector<XmlFileEntry>* files) {
    for (xmlNode* cur = node; cur != nullptr; cur = cur->next) {
        if (cur->type != XML_ELEMENT_NODE) {
            continue;
        }

        const std::string node_name = XmlCharToString(cur->name);
        if (node_name == "Directory") {
            xmlChar* dir_name = xmlGetProp(cur, reinterpret_cast<const xmlChar*>("name"));
            if (dir_name != nullptr) {
                directories->push_back(XmlCharToString(dir_name));
                xmlFree(dir_name);
            }
            WalkXml(cur->children, directories, files);
            if (!directories->empty()) {
                directories->pop_back();
            }
            continue;
        }

        if (node_name == "File") {
            xmlChar* file_name = xmlGetProp(cur, reinterpret_cast<const xmlChar*>("name"));
            if (file_name == nullptr) {
                continue;
            }

            std::filesystem::path full_path;
            for (const std::string& dir : *directories) {
                full_path /= dir;
            }
            full_path /= XmlCharToString(file_name);

            xmlChar* header_attr = xmlGetProp(cur, reinterpret_cast<const xmlChar*>("header"));
            bool is_header = false;
            if (header_attr != nullptr) {
                std::string header_value = XmlCharToString(header_attr);
                is_header = (header_value == "true" || header_value == "1");
                xmlFree(header_attr);
            }

            files->push_back({full_path.string(), is_header});
            xmlFree(file_name);
            continue;
        }

        WalkXml(cur->children, directories, files);
    }
}

bool IsSourceFile(const std::filesystem::path& path) {
    const std::string ext = path.extension().string();
    return ext == ".c" || ext == ".cc" || ext == ".cpp" || ext == ".cxx";
}

void AddListEntry(v8::Isolate* isolate,
                  v8::Local<v8::Context> context,
                  v8::Local<v8::Array> entries,
                  uint32_t index,
                  const std::string& path,
                  const std::string& header) {
    v8::Local<v8::Object> item = v8::Object::New(isolate);
    (void)item
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "tool"),
              v8::String::NewFromUtf8Literal(isolate, "absolute"))
        .FromMaybe(false);
    (void)item
        ->Set(context,
              v8::String::NewFromUtf8Literal(isolate, "path"),
              v8::String::NewFromUtf8(isolate, path.c_str()).ToLocalChecked())
        .FromMaybe(false);

    if (!header.empty()) {
        (void)item
            ->Set(context,
                  v8::String::NewFromUtf8Literal(isolate, "header"),
                  v8::String::NewFromUtf8(isolate, header.c_str()).ToLocalChecked())
            .FromMaybe(false);
    }

    (void)entries->Set(context, index, item).FromMaybe(false);
}

void ListAddCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsObject()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "Add expects entry object")));
        return;
    }

    v8::Local<v8::Object> self = args.This();
    v8::Local<v8::Value> entries_value;
    if (!self->Get(context, v8::String::NewFromUtf8Literal(isolate, "__entries")).ToLocal(&entries_value) ||
        !entries_value->IsArray()) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Container list is corrupted")));
        return;
    }

    v8::Local<v8::Array> entries = entries_value.As<v8::Array>();
    (void)entries->Set(context, entries->Length(), args[0]).FromMaybe(false);
}

void ThrowListCorrupted(v8::Isolate* isolate) {
    isolate->ThrowException(v8::Exception::Error(
        v8::String::NewFromUtf8Literal(isolate, "Container list is corrupted")));
}

bool GetListEntries(v8::Isolate* isolate,
                    v8::Local<v8::Context> context,
                    v8::Local<v8::Object> self,
                    v8::Local<v8::Array>* out_entries) {
    v8::Local<v8::Value> entries_value;
    if (!self->Get(context, v8::String::NewFromUtf8Literal(isolate, "__entries")).ToLocal(&entries_value) ||
        !entries_value->IsArray()) {
        return false;
    }
    *out_entries = entries_value.As<v8::Array>();
    return true;
}

void ListAddManyCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsArray()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "AddMany expects array of entries")));
        return;
    }

    v8::Local<v8::Array> entries;
    if (!GetListEntries(isolate, context, args.This(), &entries)) {
        ThrowListCorrupted(isolate);
        return;
    }

    v8::Local<v8::Array> input = args[0].As<v8::Array>();
    for (uint32_t i = 0; i < input->Length(); ++i) {
        v8::Local<v8::Value> entry;
        if (!input->Get(context, i).ToLocal(&entry) || !entry->IsObject()) {
            continue;
        }
        (void)entries->Set(context, entries->Length(), entry).FromMaybe(false);
    }
}

void ListClearCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    const bool ok = args.This()
                        ->Set(context,
                              v8::String::NewFromUtf8Literal(isolate, "__entries"),
                              v8::Array::New(isolate))
                        .FromMaybe(false);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void ListSizeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::Local<v8::Array> entries;
    if (!GetListEntries(isolate, context, args.This(), &entries)) {
        ThrowListCorrupted(isolate);
        return;
    }

    args.GetReturnValue().Set(v8::Integer::New(isolate, entries->Length()));
}

void ListEntriesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::Local<v8::Array> entries;
    if (!GetListEntries(isolate, context, args.This(), &entries)) {
        ThrowListCorrupted(isolate);
        return;
    }

    args.GetReturnValue().Set(entries);
}

void ListSetRootCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "SetRoot expects string root")));
        return;
    }

    bool ok = args.This()
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__root"),
                        args[0])
                  .FromMaybe(false);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void ListGetRootCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::Local<v8::Value> root;
    if (!args.This()->Get(context, v8::String::NewFromUtf8Literal(isolate, "__root")).ToLocal(&root)) {
        args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(isolate, ""));
        return;
    }
    args.GetReturnValue().Set(root);
}

void ListRemoveAtCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsNumber()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "RemoveAt expects index")));
        return;
    }

    v8::Local<v8::Array> entries;
    if (!GetListEntries(isolate, context, args.This(), &entries)) {
        ThrowListCorrupted(isolate);
        return;
    }

    int index = args[0].As<v8::Number>()->Value();
    if (index < 0 || static_cast<uint32_t>(index) >= entries->Length()) {
        args.GetReturnValue().Set(v8::Boolean::New(isolate, false));
        return;
    }

    for (uint32_t i = static_cast<uint32_t>(index); i + 1 < entries->Length(); ++i) {
        v8::Local<v8::Value> next;
        if (entries->Get(context, i + 1).ToLocal(&next)) {
            (void)entries->Set(context, i, next).FromMaybe(false);
        }
    }
    (void)entries->Delete(context, entries->Length() - 1).FromMaybe(false);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, true));
}

void ListDeduplicateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    v8::Local<v8::Array> entries;
    if (!GetListEntries(isolate, context, args.This(), &entries)) {
        ThrowListCorrupted(isolate);
        return;
    }

    std::set<std::string> seen;
    v8::Local<v8::Array> unique_entries = v8::Array::New(isolate);
    uint32_t out_idx = 0;

    for (uint32_t i = 0; i < entries->Length(); ++i) {
        v8::Local<v8::Value> value;
        if (!entries->Get(context, i).ToLocal(&value) || !value->IsObject()) {
            continue;
        }

        v8::Local<v8::Object> entry = value.As<v8::Object>();
        v8::Local<v8::Value> path_value;
        if (!entry->Get(context, v8::String::NewFromUtf8Literal(isolate, "path")).ToLocal(&path_value) ||
            !path_value->IsString()) {
            continue;
        }

        std::string path_key = ValueToString(isolate, path_value);
        v8::Local<v8::Value> header_value;
        if (entry->Get(context, v8::String::NewFromUtf8Literal(isolate, "header")).ToLocal(&header_value) &&
            header_value->IsString()) {
            path_key += "|" + ValueToString(isolate, header_value);
        }

        if (seen.insert(path_key).second) {
            (void)unique_entries->Set(context, out_idx++, entry).FromMaybe(false);
        }
    }

    bool ok = args.This()
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__entries"),
                        unique_entries)
                  .FromMaybe(false);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void ListAddPathCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "AddPath expects source path string")));
        return;
    }

    v8::Local<v8::Array> entries;
    if (!GetListEntries(isolate, context, args.This(), &entries)) {
        ThrowListCorrupted(isolate);
        return;
    }

    v8::Local<v8::Object> item = v8::Object::New(isolate);
    bool ok = item
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "tool"),
                        v8::String::NewFromUtf8Literal(isolate, "absolute"))
                  .FromMaybe(false);
    ok = ok && item
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "path"),
                         args[0])
                   .FromMaybe(false);

    if (args.Length() > 1 && args[1]->IsString()) {
        ok = ok && item
                       ->Set(context,
                             v8::String::NewFromUtf8Literal(isolate, "header"),
                             args[1])
                       .FromMaybe(false);
    }

    ok = ok && entries->Set(context, entries->Length(), item).FromMaybe(false);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void ListMakeFromDirectoryCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "MakeFromDirectory expects root path")));
        return;
    }

    const std::filesystem::path root = ValueToString(isolate, args[0]);
    bool recursive = true;
    if (args.Length() > 1 && args[1]->IsBoolean()) {
        recursive = args[1].As<v8::Boolean>()->Value();
    }

    std::vector<std::filesystem::path> sources;
    std::vector<std::filesystem::path> headers;
    std::error_code ec;
    if (recursive) {
        for (auto it = std::filesystem::recursive_directory_iterator(root, ec);
             !ec && it != std::filesystem::recursive_directory_iterator();
             ++it) {
            if (!it->is_regular_file()) {
                continue;
            }
            const std::filesystem::path rel = std::filesystem::relative(it->path(), root, ec);
            if (ec) {
                continue;
            }
            const std::string ext = rel.extension().string();
            if (ext == ".h" || ext == ".hpp" || ext == ".hh") {
                headers.push_back(rel);
            }
            if (IsSourceFile(rel)) {
                sources.push_back(rel);
            }
        }
    } else {
        for (auto it = std::filesystem::directory_iterator(root, ec);
             !ec && it != std::filesystem::directory_iterator();
             ++it) {
            if (!it->is_regular_file()) {
                continue;
            }
            const std::filesystem::path rel = it->path().filename();
            const std::string ext = rel.extension().string();
            if (ext == ".h" || ext == ".hpp" || ext == ".hh") {
                headers.push_back(rel);
            }
            if (IsSourceFile(rel)) {
                sources.push_back(rel);
            }
        }
    }

    v8::Local<v8::Array> entries = v8::Array::New(isolate);
    uint32_t index = 0;
    for (const auto& source : sources) {
        const std::string stem = source.stem().string();
        std::string header_name;
        for (const auto& header : headers) {
            if (header.stem().string() == stem) {
                header_name = header.string();
                break;
            }
        }
        AddListEntry(isolate, context, entries, index++, source.string(), header_name);
    }

    v8::Local<v8::Object> list = v8::Object::New(isolate);
    bool ok = list
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__root"),
                        v8::String::NewFromUtf8(isolate, root.string().c_str()).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "__entries"),
                         entries)
                   .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "Add"),
                         v8::Function::New(context, ListAddCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "AddMany"),
                         v8::Function::New(context, ListAddManyCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "AddPath"),
                         v8::Function::New(context, ListAddPathCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "Clear"),
                         v8::Function::New(context, ListClearCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "Size"),
                         v8::Function::New(context, ListSizeCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "Entries"),
                         v8::Function::New(context, ListEntriesCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "SetRoot"),
                         v8::Function::New(context, ListSetRootCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "GetRoot"),
                         v8::Function::New(context, ListGetRootCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "RemoveAt"),
                         v8::Function::New(context, ListRemoveAtCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "Deduplicate"),
                         v8::Function::New(context, ListDeduplicateCallback).ToLocalChecked())
                   .FromMaybe(false);

    if (!ok) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to create list from directory")));
        return;
    }

    args.GetReturnValue().Set(list);
}

void CreateListCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsObject()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "CreateList expects options object")));
        return;
    }

    v8::Local<v8::Object> options = args[0].As<v8::Object>();
    v8::Local<v8::Value> root_value;
    if (!options->Get(context, v8::String::NewFromUtf8Literal(isolate, "root")).ToLocal(&root_value) ||
        !root_value->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "root must be string")));
        return;
    }

    v8::Local<v8::Object> list = v8::Object::New(isolate);
    bool ok = list
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__root"),
                        root_value)
                  .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "__entries"),
                         v8::Array::New(isolate))
                   .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "Add"),
                         v8::Function::New(context, ListAddCallback).ToLocalChecked())
                   .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "AddMany"),
                     v8::Function::New(context, ListAddManyCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "AddPath"),
                     v8::Function::New(context, ListAddPathCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Clear"),
                     v8::Function::New(context, ListClearCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Size"),
                     v8::Function::New(context, ListSizeCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Entries"),
                     v8::Function::New(context, ListEntriesCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "SetRoot"),
                     v8::Function::New(context, ListSetRootCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "GetRoot"),
                     v8::Function::New(context, ListGetRootCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "RemoveAt"),
                     v8::Function::New(context, ListRemoveAtCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Deduplicate"),
                     v8::Function::New(context, ListDeduplicateCallback).ToLocalChecked())
                 .FromMaybe(false);

    if (!ok) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to create list")));
        return;
    }

    args.GetReturnValue().Set(list);
}

void MakeFromXmlCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "MakeFromXML expects XML path")));
        return;
    }

    std::string xml_path = ValueToString(isolate, args[0]);
    std::filesystem::path selected_xml(xml_path);
    if (!std::filesystem::exists(selected_xml)) {
        std::filesystem::path fallback = std::filesystem::path("example") / xml_path;
        if (std::filesystem::exists(fallback)) {
            selected_xml = fallback;
        }
    }

    xmlDocPtr doc = xmlReadFile(selected_xml.string().c_str(), nullptr, XML_PARSE_NONET);

    if (doc == nullptr) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Unable to read XML list")));
        return;
    }

    std::vector<std::string> directories;
    std::vector<XmlFileEntry> files;
    xmlNode* root = xmlDocGetRootElement(doc);
    WalkXml(root, &directories, &files);
    xmlFreeDoc(doc);

    std::vector<std::filesystem::path> source_paths;
    std::vector<std::filesystem::path> header_paths;
    for (const XmlFileEntry& file : files) {
        std::filesystem::path path(file.path);
        if (file.is_header || path.extension() == ".h" || path.extension() == ".hpp") {
            header_paths.push_back(path);
        }
        if (IsSourceFile(path)) {
            source_paths.push_back(path);
        }
    }

    if (source_paths.empty()) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "XML does not contain source files")));
        return;
    }

    std::filesystem::path root_dir = source_paths.front().parent_path();
    v8::Local<v8::Array> entries = v8::Array::New(isolate);

    for (uint32_t i = 0; i < source_paths.size(); ++i) {
        const std::filesystem::path& source_path = source_paths[i];
        const std::string stem = source_path.stem().string();
        std::string header_name;

        for (const auto& header_path : header_paths) {
            if (header_path.stem().string() == stem) {
                header_name = header_path.filename().string();
                break;
            }
        }

        AddListEntry(isolate,
                     context,
                     entries,
                     i,
                     source_path.filename().string(),
                     header_name);
    }

    v8::Local<v8::Object> list = v8::Object::New(isolate);
    bool ok = list
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "__root"),
                        v8::String::NewFromUtf8(isolate, root_dir.string().c_str()).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "__entries"),
                         entries)
                   .FromMaybe(false);
    ok = ok && list
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "Add"),
                         v8::Function::New(context, ListAddCallback).ToLocalChecked())
                   .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "AddMany"),
                     v8::Function::New(context, ListAddManyCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "AddPath"),
                     v8::Function::New(context, ListAddPathCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Clear"),
                     v8::Function::New(context, ListClearCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Size"),
                     v8::Function::New(context, ListSizeCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Entries"),
                     v8::Function::New(context, ListEntriesCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "SetRoot"),
                     v8::Function::New(context, ListSetRootCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "GetRoot"),
                     v8::Function::New(context, ListGetRootCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "RemoveAt"),
                     v8::Function::New(context, ListRemoveAtCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && list
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "Deduplicate"),
                     v8::Function::New(context, ListDeduplicateCallback).ToLocalChecked())
                 .FromMaybe(false);

    if (!ok) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "Failed to build XML list")));
        return;
    }

    args.GetReturnValue().Set(list);
}

}  // namespace

namespace modules {

bool RegisterContainerModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> module = v8::Object::New(isolate);
    bool ok = module
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "CreateList"),
                        v8::Function::New(context, CreateListCallback).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && module
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "MakeFromXML"),
                         v8::Function::New(context, MakeFromXmlCallback).ToLocalChecked())
                   .FromMaybe(false);
        ok = ok && module
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "MakeFromDirectory"),
                     v8::Function::New(context, ListMakeFromDirectoryCallback).ToLocalChecked())
                 .FromMaybe(false);
    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Container"), module)
        .FromMaybe(false);
}

}  // namespace modules
