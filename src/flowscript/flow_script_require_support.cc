#include "flow_script_require_support.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <system_error>
#include <thread>
#include <utility>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <openssl/sha.h>

#include "engine_params.h"
#include "javascript/common/compression_codec.h"
#include "helper/tool_to.h"
#include <iostream>

namespace flow_script_detail::require_support {

namespace {

std::filesystem::path LockPathForScript(const std::filesystem::path& script_path) {
    const std::string lock_name = script_path.filename().string() + ".flowcache.lock";
    return script_path.parent_path() / lock_name;
}

std::filesystem::path TempPathForTarget(const std::filesystem::path& target,
                                        const char* extension_tag) {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const auto ticks = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
    std::filesystem::path tmp = target;
    tmp += ".tmp.";
    tmp += extension_tag;
    tmp += ".";
    tmp += std::to_string(ticks);
    return tmp;
}

bool CommitTempFile(const std::filesystem::path& tmp,
                    const std::filesystem::path& target,
                    std::string* error_out) {
    std::error_code ec;
    std::filesystem::rename(tmp, target, ec);
    if (!ec) {
        return true;
    }

    std::filesystem::remove(target, ec);
    ec.clear();
    std::filesystem::rename(tmp, target, ec);
    if (!ec) {
        return true;
    }

    if (error_out != nullptr) {
        *error_out = "Failed to commit file: " + target.string() + ", reason: " + ec.message();
    }
    std::filesystem::remove(tmp, ec);
    return false;
}

std::string BoolToString(bool value) {
    return value ? "true" : "false";
}

xmlNodePtr AddChildNode(xmlNodePtr parent,
                        const char* name,
                        const std::string& value) {
    return xmlNewChild(parent, nullptr, BAD_CAST name, BAD_CAST value.c_str());
}

xmlNodePtr FindFirstChild(xmlNodePtr parent, const char* name) {
    if (parent == nullptr) {
        return nullptr;
    }

    for (xmlNodePtr node = parent->children; node != nullptr; node = node->next) {
        if (node->type == XML_ELEMENT_NODE && xmlStrEqual(node->name, BAD_CAST name)) {
            return node;
        }
    }
    return nullptr;
}

std::string NodeText(xmlNodePtr node) {
    if (node == nullptr) {
        return {};
    }

    xmlChar* content = xmlNodeGetContent(node);
    if (content == nullptr) {
        return {};
    }

    std::string text(reinterpret_cast<const char*>(content));
    xmlFree(content);
    return text;
}

std::uintmax_t FileSizeOrZero(const std::filesystem::path& path) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(path, ec);
    if (ec) {
        return 0;
    }
    return size;
}

std::string FileWriteTimeOrNA(const std::filesystem::path& path) {
    std::error_code ec;
    const auto ftime = std::filesystem::last_write_time(path, ec);
    if (ec) {
        return "n/a";
    }
    const auto ticks = ftime.time_since_epoch().count();
    return std::to_string(ticks);
}

std::string Sha256HexRaw(const std::uint8_t* data, std::size_t size) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(data, size, digest);

    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.resize(SHA256_DIGEST_LENGTH * 2);
    for (std::size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        out[i * 2] = kHex[(digest[i] >> 4) & 0x0F];
        out[i * 2 + 1] = kHex[digest[i] & 0x0F];
    }
    return out;
}

std::uint64_t ParseU64OrZero(const std::string& text) {
    if (text.empty()) {
        return 0;
    }

    try {
        return static_cast<std::uint64_t>(std::stoull(text));
    } catch (...) {
        return 0;
    }
}

std::string NowTicksString() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

std::string ShellEscape(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 2);
    out.push_back('\'');
    for (char c : input) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out.push_back(c);
        }
    }
    out.push_back('\'');
    return out;
}

bool CompileTypeScriptToJavaScript(const std::filesystem::path& ts_path,
                                   const std::filesystem::path& js_path,
                                   std::string* compiler_used,
                                   std::string* error_out) {
    std::error_code ec;
    std::filesystem::create_directories(js_path.parent_path(), ec);

    const EngineParams params = EngineParamsFromEnv();

    const std::string ts_dir  = ts_path.parent_path().string();
    const std::string ts_file = ts_path.filename().string();

    // Determine effective TS target (CLI override or default).
    const std::string ts_target = params.ts_target.empty() ? "ES2020" : params.ts_target;

    // Suppress compiler output unless --details_compiler is set.
    const std::string redirect = params.details_compiler ? "" : " >/dev/null 2>&1";

    // Build extra tsc flags from params.
    std::string extra_tsc;
    if (params.ts_strict)   extra_tsc += " --strict";
    if (params.ts_no_check) extra_tsc += " --noCheck";

    const std::string tsc_cmd =
        "cd " + ShellEscape(ts_dir)
        + " && tsc --pretty false --skipLibCheck --target " + ts_target
        + " --module commonjs --outDir . " + ShellEscape(ts_file)
        + extra_tsc + redirect;

    const std::string npx_tsc_cmd =
        "cd " + ShellEscape(ts_dir)
        + " && npx --yes tsc --pretty false --skipLibCheck --target " + ts_target
        + " --module commonjs --outDir . " + ShellEscape(ts_file)
        + extra_tsc + redirect;

    const std::string esbuild_cmd =
        "cd " + ShellEscape(ts_dir)
        + " && npx --yes esbuild " + ShellEscape(ts_file)
        + " --platform=node --format=cjs --target=" + ts_target
        + " --outfile=" + ShellEscape(js_path.filename().string())
        + redirect;

    // If a specific compiler was requested, only try that one.
    const std::string& forced = params.ts_compiler;

    auto try_tsc = [&]() -> bool {
        if (params.is_verbose()) std::cerr << "[ts] trying tsc: " << tsc_cmd << "\n";
        const int rc = std::system(tsc_cmd.c_str());
        const bool ok = (rc == 0 && std::filesystem::exists(js_path));
        if (ok && compiler_used) *compiler_used = "tsc";
        return ok;
    };
    auto try_npx_tsc = [&]() -> bool {
        if (params.is_verbose()) std::cerr << "[ts] trying npx-tsc: " << npx_tsc_cmd << "\n";
        const int rc = std::system(npx_tsc_cmd.c_str());
        const bool ok = (rc == 0 && std::filesystem::exists(js_path));
        if (ok && compiler_used) *compiler_used = "npx-tsc";
        return ok;
    };
    auto try_esbuild = [&]() -> bool {
        if (params.is_verbose()) std::cerr << "[ts] trying esbuild: " << esbuild_cmd << "\n";
        const int rc = std::system(esbuild_cmd.c_str());
        const bool ok = (rc == 0 && std::filesystem::exists(js_path));
        if (ok && compiler_used) *compiler_used = "esbuild";
        return ok;
    };

    bool built = false;
    if (forced == "tsc") {
        built = try_tsc();
    } else if (forced == "npx-tsc") {
        built = try_npx_tsc();
    } else if (forced == "esbuild") {
        built = try_esbuild();
    } else {
        // Default fallback chain: tsc → npx-tsc → esbuild.
        built = try_tsc() || try_npx_tsc() || try_esbuild();
    }

    if (!built) {
        if (error_out != nullptr) {
            const std::string tried = forced.empty() ? "tsc, npx tsc, npx esbuild" : forced;
            *error_out = "TypeScript compile failed for: " + ts_path.string()
                + ". Tried: " + tried + ".";
        }
        return false;
    }

    if (!std::filesystem::exists(js_path)) {
        if (error_out != nullptr) {
            *error_out = "TypeScript compile finished but JS output missing: " + js_path.string();
        }
        return false;
    }

    return true;
}

bool EnsureCompiledJavaScript(const std::filesystem::path& source_path,
                              const std::filesystem::path& compiled_js_path,
                              std::string* compiler_used,
                              std::string* error_out) {
    std::error_code ec;
    const bool compiled_exists = std::filesystem::exists(compiled_js_path, ec) && !ec;
    if (compiled_exists) {
        const auto src_time = std::filesystem::last_write_time(source_path, ec);
        if (!ec) {
            const auto out_time = std::filesystem::last_write_time(compiled_js_path, ec);
            if (!ec && out_time >= src_time) {
                if (compiler_used != nullptr) {
                    *compiler_used = "cached-js";
                }
                return true;
            }
        }
    }

    return CompileTypeScriptToJavaScript(source_path, compiled_js_path, compiler_used, error_out);
}

xmlDocPtr BuildCacheMetadataXml(const CacheMetadata& m) {
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    if (doc == nullptr) {
        return nullptr;
    }

    xmlNodePtr root = xmlNewNode(nullptr, BAD_CAST "flow_script_require_cache");
    if (root == nullptr) {
        xmlFreeDoc(doc);
        return nullptr;
    }
    xmlDocSetRootElement(doc, root);

    xmlNodePtr script = xmlNewChild(root, nullptr, BAD_CAST "script", nullptr);
    AddChildNode(script, "path", m.script_path.string());
    AddChildNode(script, "entry_path", m.entry_path.string());
    AddChildNode(script, "parent_path", m.parent_path);
    AddChildNode(script, "source_kind", m.script_source_kind);
    AddChildNode(script, "typescript_compiler", m.typescript_compiler);
    AddChildNode(script, "size", std::to_string(FileSizeOrZero(m.script_path)));
    AddChildNode(script, "last_write_time_ticks", FileWriteTimeOrNA(m.script_path));
    AddChildNode(script, "sha256", m.source_sha256);

    xmlNodePtr require_node = xmlNewChild(root, nullptr, BAD_CAST "require", nullptr);
    AddChildNode(require_node, "stack_depth", std::to_string(m.require_stack_paths.size()));
    xmlNodePtr require_stack = xmlNewChild(require_node, nullptr, BAD_CAST "stack", nullptr);
    for (const auto& required_path : m.require_stack_paths) {
        AddChildNode(require_stack, "path", required_path);
    }
    xmlNodePtr cached_modules = xmlNewChild(require_node, nullptr, BAD_CAST "cached_modules", nullptr);
    AddChildNode(cached_modules, "count", std::to_string(m.cached_module_keys.size()));
    for (const auto& key : m.cached_module_keys) {
        AddChildNode(cached_modules, "module", key);
    }

    xmlNodePtr cache = xmlNewChild(root, nullptr, BAD_CAST "cache", nullptr);
    AddChildNode(cache, "path", m.cache_path.string());
    AddChildNode(cache, "exists", BoolToString(m.cache_exists));
    AddChildNode(cache, "valid", BoolToString(m.cache_valid));
    AddChildNode(cache, "rebuilt", BoolToString(m.cache_rebuilt));
    AddChildNode(cache, "used", BoolToString(m.used_cache));
    AddChildNode(cache, "size", std::to_string(FileSizeOrZero(m.cache_path)));
    AddChildNode(cache, "last_write_time_ticks", FileWriteTimeOrNA(m.cache_path));
    AddChildNode(cache, "sha256", m.cache_binary_sha256);
    AddChildNode(cache, "expanded_sha256", m.compiled_sha256);
    AddChildNode(cache, "validation_mode", m.validation_mode);
    AddChildNode(cache, "validation_reason", m.validation_reason);
    xmlNodePtr validation_stats = xmlNewChild(cache, nullptr, BAD_CAST "validation_stats", nullptr);
    AddChildNode(validation_stats, "total", std::to_string(m.validation_counters.total));
    AddChildNode(validation_stats, "hash", std::to_string(m.validation_counters.hash));
    AddChildNode(validation_stats, "diff", std::to_string(m.validation_counters.diff));
    AddChildNode(validation_stats, "rebuild", std::to_string(m.validation_counters.rebuild));
    xmlNodePtr validation_history = xmlNewChild(cache, nullptr, BAD_CAST "validation_history", nullptr);
    AddChildNode(validation_history, "count", std::to_string(m.validation_history.size()));
    for (const auto& event : m.validation_history) {
        xmlNodePtr entry = xmlNewChild(validation_history, nullptr, BAD_CAST "validation", nullptr);
        AddChildNode(entry, "mode", event.mode);
        AddChildNode(entry, "reason", event.reason);
        AddChildNode(entry, "status", event.status);
        AddChildNode(entry, "timestamp_ticks", event.timestamp_ticks);
    }

    xmlNodePtr diff_node = xmlNewChild(root, nullptr, BAD_CAST "diff", nullptr);
    AddChildNode(diff_node, "equal", BoolToString(m.diff.equal));
    AddChildNode(diff_node, "added_count", std::to_string(m.diff.added_count));
    AddChildNode(diff_node, "removed_count", std::to_string(m.diff.removed_count));
    xmlNodePtr operations = xmlNewChild(diff_node, nullptr, BAD_CAST "operations", nullptr);
    for (const auto& op : m.diff.operations) {
        const char* sign = (op.type == FileDiffOperationType::Added) ? "+" : "-";
        xmlNodePtr op_node = xmlNewChild(operations, nullptr, BAD_CAST "op", BAD_CAST op.line.c_str());
        xmlNewProp(op_node, BAD_CAST "sign", BAD_CAST sign);
    }

    xmlNodePtr runtime = xmlNewChild(root, nullptr, BAD_CAST "runtime", nullptr);
    AddChildNode(runtime, "status", m.status);
    AddChildNode(runtime, "error", m.error_message);

    return doc;
}

}  // namespace

ScriptCacheLock::ScriptCacheLock(std::filesystem::path lock_path)
    : lock_path_(std::move(lock_path)), locked_(true) {}

ScriptCacheLock::~ScriptCacheLock() {
    if (!locked_) {
        return;
    }
    std::error_code ec;
    std::filesystem::remove_all(lock_path_, ec);
}

std::string ToUtf8(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    if (value.IsEmpty()) {
        return {};
    }

    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return {};
    }
    return *utf8;
}

std::filesystem::path CachePathForScript(const std::filesystem::path& script_path) {
    const std::string cache_name = script_path.filename().string() + ".flowcache.bin";
    return script_path.parent_path() / cache_name;
}

std::filesystem::path MetadataPathForScript(const std::filesystem::path& script_path) {
    const std::string meta_name = script_path.filename().string() + ".flowcache.xml";
    return script_path.parent_path() / meta_name;
}

std::filesystem::path DiffTextPathForScript(const std::filesystem::path& script_path) {
    const std::string diff_name = script_path.filename().string() + ".flowcache.diff";
    return script_path.parent_path() / diff_name;
}

std::unique_ptr<ScriptCacheLock> AcquireScriptCacheLock(const std::filesystem::path& script_path,
                                                        std::string* error_out) {
    const std::filesystem::path lock_path = LockPathForScript(script_path);

    constexpr int kMaxAttempts = 100;
    constexpr auto kRetryDelay = std::chrono::milliseconds(20);
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        std::error_code ec;
        if (std::filesystem::create_directory(lock_path, ec)) {
            return std::make_unique<ScriptCacheLock>(lock_path);
        }

        if (ec && error_out != nullptr) {
            *error_out = "Failed to create lock: " + lock_path.string() + ", reason: " + ec.message();
            return nullptr;
        }

        std::this_thread::sleep_for(kRetryDelay);
    }

    if (error_out != nullptr) {
        *error_out = "Timed out waiting for lock: " + lock_path.string();
    }
    return nullptr;
}

bool PrepareScriptForExecution(const std::filesystem::path& source_path,
                               ScriptExecutionInfo* execution_info,
                               std::string* error_out) {
    if (execution_info == nullptr) {
        if (error_out != nullptr) {
            *error_out = "PrepareScriptForExecution: execution_info is null";
        }
        return false;
    }

    const auto ext = source_path.extension().string();
    if (ext != ".ts" && ext != ".tsx") {
        execution_info->executable_path = source_path;
        execution_info->source_kind = "js";
        execution_info->compiler_used = "none";
        return true;
    }

    std::filesystem::path compiled_js = source_path;
    compiled_js.replace_extension(".js");

    std::string compiler_used;
    if (!EnsureCompiledJavaScript(source_path, compiled_js, &compiler_used, error_out)) {
        return false;
    }

    execution_info->executable_path = compiled_js;
    execution_info->source_kind = "ts";
    execution_info->compiler_used = compiler_used.empty() ? "unknown" : compiler_used;
    return true;
}

std::string Sha256Hex(const std::string& text) {
    return Sha256HexRaw(reinterpret_cast<const std::uint8_t*>(text.data()), text.size());
}

std::string Sha256Hex(const std::vector<std::uint8_t>& bytes) {
    return Sha256HexRaw(bytes.data(), bytes.size());
}

bool ReadCacheMetadataHashes(const std::filesystem::path& metadata_path,
                             CachedMetadataHashes* out_hashes) {
    if (out_hashes == nullptr || !std::filesystem::exists(metadata_path)) {
        return false;
    }

    xmlDocPtr doc = xmlReadFile(metadata_path.string().c_str(), nullptr, XML_PARSE_NONET);
    if (doc == nullptr) {
        return false;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (root == nullptr || !xmlStrEqual(root->name, BAD_CAST "flow_script_require_cache")) {
        xmlFreeDoc(doc);
        return false;
    }

    xmlNodePtr script = FindFirstChild(root, "script");
    xmlNodePtr cache = FindFirstChild(root, "cache");
    if (script == nullptr || cache == nullptr) {
        xmlFreeDoc(doc);
        return false;
    }

    out_hashes->script_sha256 = NodeText(FindFirstChild(script, "sha256"));
    out_hashes->expanded_sha256 = NodeText(FindFirstChild(cache, "expanded_sha256"));
    out_hashes->cache_sha256 = NodeText(FindFirstChild(cache, "sha256"));

    xmlFreeDoc(doc);
    return !out_hashes->script_sha256.empty() && !out_hashes->expanded_sha256.empty();
}

bool ReadValidationCounters(const std::filesystem::path& metadata_path,
                            ValidationCounters* counters) {
    if (counters == nullptr || !std::filesystem::exists(metadata_path)) {
        return false;
    }

    xmlDocPtr doc = xmlReadFile(metadata_path.string().c_str(), nullptr, XML_PARSE_NONET);
    if (doc == nullptr) {
        return false;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (root == nullptr || !xmlStrEqual(root->name, BAD_CAST "flow_script_require_cache")) {
        xmlFreeDoc(doc);
        return false;
    }

    xmlNodePtr cache = FindFirstChild(root, "cache");
    xmlNodePtr stats = FindFirstChild(cache, "validation_stats");
    if (cache == nullptr || stats == nullptr) {
        xmlFreeDoc(doc);
        return false;
    }

    counters->total = ParseU64OrZero(NodeText(FindFirstChild(stats, "total")));
    counters->hash = ParseU64OrZero(NodeText(FindFirstChild(stats, "hash")));
    counters->diff = ParseU64OrZero(NodeText(FindFirstChild(stats, "diff")));
    counters->rebuild = ParseU64OrZero(NodeText(FindFirstChild(stats, "rebuild")));

    xmlFreeDoc(doc);
    return true;
}

bool ReadValidationHistory(const std::filesystem::path& metadata_path,
                           ValidationHistory* history) {
    if (history == nullptr || !std::filesystem::exists(metadata_path)) {
        return false;
    }

    xmlDocPtr doc = xmlReadFile(metadata_path.string().c_str(), nullptr, XML_PARSE_NONET);
    if (doc == nullptr) {
        return false;
    }

    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (root == nullptr || !xmlStrEqual(root->name, BAD_CAST "flow_script_require_cache")) {
        xmlFreeDoc(doc);
        return false;
    }

    xmlNodePtr cache = FindFirstChild(root, "cache");
    xmlNodePtr validation_history = FindFirstChild(cache, "validation_history");
    if (cache == nullptr || validation_history == nullptr) {
        xmlFreeDoc(doc);
        return false;
    }

    history->clear();
    for (xmlNodePtr node = validation_history->children; node != nullptr; node = node->next) {
        if (node->type != XML_ELEMENT_NODE || !xmlStrEqual(node->name, BAD_CAST "validation")) {
            continue;
        }

        ValidationEvent event;
        event.mode = NodeText(FindFirstChild(node, "mode"));
        event.reason = NodeText(FindFirstChild(node, "reason"));
        event.status = NodeText(FindFirstChild(node, "status"));
        event.timestamp_ticks = NodeText(FindFirstChild(node, "timestamp_ticks"));
        history->push_back(std::move(event));
    }

    xmlFreeDoc(doc);
    return !history->empty();
}

void IncrementValidationCounter(ValidationCounters* counters, const std::string& mode) {
    if (counters == nullptr) {
        return;
    }

    ++counters->total;
    if (mode == "hash") {
        ++counters->hash;
    } else if (mode == "diff") {
        ++counters->diff;
    } else {
        ++counters->rebuild;
    }
}

void PushValidationEvent(ValidationHistory* history,
                         const std::string& mode,
                         const std::string& reason,
                         const std::string& status) {
    if (history == nullptr) {
        return;
    }

    history->push_back(ValidationEvent{mode, reason, status, NowTicksString()});

    constexpr std::size_t kMaxHistory = 10;
    if (history->size() > kMaxHistory) {
        const auto trim = history->size() - kMaxHistory;
        history->erase(history->begin(), history->begin() + static_cast<std::ptrdiff_t>(trim));
    }
}

bool WriteCacheMetadataFile(const std::filesystem::path& meta_path,
                            const CacheMetadata& metadata,
                            std::string* error_out) {
    xmlDocPtr doc = BuildCacheMetadataXml(metadata);
    if (doc == nullptr) {
        if (error_out != nullptr) {
            *error_out = "Failed to build XML metadata document";
        }
        return false;
    }

    const std::filesystem::path tmp_path = TempPathForTarget(meta_path, "xml");
    const int rc = xmlSaveFormatFileEnc(tmp_path.string().c_str(), doc, "UTF-8", 1);
    xmlFreeDoc(doc);

    if (rc < 0) {
        if (error_out != nullptr) {
            *error_out = "Failed to save XML metadata: " + meta_path.string();
        }
        return false;
    }

    return CommitTempFile(tmp_path, meta_path, error_out);
}

bool WriteDiffText(const std::filesystem::path& diff_path,
                   const FileDiffReturn& diff,
                   std::string* error_out) {
    std::string text;
    text.reserve(256 + diff.operations.size() * 32);
    text += "equal=" + BoolToString(diff.equal) + "\n";
    text += "added_count=" + std::to_string(diff.added_count) + "\n";
    text += "removed_count=" + std::to_string(diff.removed_count) + "\n";
    text += "operations:\n";
    for (const auto& op : diff.operations) {
        const char sign = (op.type == FileDiffOperationType::Added) ? '+' : '-';
        text.push_back(sign);
        text.push_back(' ');
        text += op.line;
        text.push_back('\n');
    }

    const std::filesystem::path tmp_path = TempPathForTarget(diff_path, "txt");
    if (!ToolTo::WriteTextFile(tmp_path, text, false, error_out)) {
        return false;
    }
    return CommitTempFile(tmp_path, diff_path, error_out);
}

bool ReadBinaryFile(const std::filesystem::path& path,
                    std::vector<std::uint8_t>* bytes,
                    std::string* error_out) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        if (error_out != nullptr) {
            *error_out = "Failed to open cache file: " + path.string();
        }
        return false;
    }

    in.seekg(0, std::ios::end);
    const std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    if (size < 0) {
        if (error_out != nullptr) {
            *error_out = "Failed to read cache file size: " + path.string();
        }
        return false;
    }

    bytes->assign(static_cast<std::size_t>(size), 0);
    if (size > 0 && !in.read(reinterpret_cast<char*>(bytes->data()), size)) {
        if (error_out != nullptr) {
            *error_out = "Failed to read cache file bytes: " + path.string();
        }
        return false;
    }
    return true;
}

bool WriteBinaryFile(const std::filesystem::path& path,
                     const std::vector<std::uint8_t>& bytes,
                     std::string* error_out) {
    const std::filesystem::path tmp_path = TempPathForTarget(path, "bin");
    std::ofstream out(tmp_path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        if (error_out != nullptr) {
            *error_out = "Failed to create cache temp file: " + tmp_path.string();
        }
        return false;
    }

    if (!bytes.empty()) {
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!out.good()) {
            if (error_out != nullptr) {
                *error_out = "Failed to write cache temp file: " + tmp_path.string();
            }
            return false;
        }
    }

    out.flush();
    if (!out.good()) {
        if (error_out != nullptr) {
            *error_out = "Failed to flush cache temp file: " + tmp_path.string();
        }
        return false;
    }
    out.close();

    return CommitTempFile(tmp_path, path, error_out);
}

bool ReadExpandedTextFromCache(const std::filesystem::path& cache_path,
                               std::string* text,
                               std::string* error_out) {
    std::vector<std::uint8_t> compressed;
    if (!ReadBinaryFile(cache_path, &compressed, error_out)) {
        return false;
    }

    auto decompressed = engine::javascript::common::CompressionCodec::DecompressToString(compressed);
    if (!decompressed.has_value()) {
        if (error_out != nullptr) {
            *error_out = "Failed to decompress cache file: " + cache_path.string();
        }
        return false;
    }

    *text = *decompressed;
    return true;
}

bool RebuildCacheFromExpandedText(const std::filesystem::path& cache_path,
                                  const std::string& expanded_text,
                                  std::string* error_out) {
    auto compressed = engine::javascript::common::CompressionCodec::Compress(expanded_text);
    if (!compressed.has_value()) {
        if (error_out != nullptr) {
            *error_out = "Failed to compress script for cache: " + cache_path.string();
        }
        return false;
    }

    return WriteBinaryFile(cache_path, *compressed, error_out);
}

}  // namespace flow_script_detail::require_support
