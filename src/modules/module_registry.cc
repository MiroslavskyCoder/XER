#include "modules/module_registry.h"

#include "async_io/async_file_reader.h"
#include "async_io/async_file_writer.h"
#include "compiler_source.h"
#include "doctor/analysis/analysis_engine.h"
#include "doctor/analysis/analysis_result_handler.h"
#include "doctor/core/engine_doctor_config.h"
#include "doctor/core/engine_doctor_context.h"
#include "doctor/scanner/scanner_module.h"
#include "doctor/scanner/scan_parameters.h"
#include "doctor/scanner/scan_result.h"
#include "helper/class_builder.h"
#include "helper/module_builder.h"
#include "helper/tool_to.h"
#include "provider.h"
#include "runtime_live.h"
#include "wrapper/angle/angle_engine_bridge.h"
#include "wrapper/cuda/cuda_engine_bridge.h"
#include "wrapper/cudnn/cudnn_engine_bridge.h"
#include "wrapper/ffmpeg/ffmpeg_engine_bridge.h"
#include "wrapper/opencv/opencv_engine_bridge.h"
#include "wrapper/skia/skia_engine_bridge.h"

#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace modules {
namespace {

std::string NormalizeModuleKey(const std::string& text) {
	std::string out;
	out.reserve(text.size());
	for (unsigned char ch : text) {
		if (std::isalnum(ch) != 0) {
			out.push_back(static_cast<char>(std::tolower(ch)));
		}
	}
	return out;
}

std::string ToLowerCopy(const std::string& text) {
	std::string out = text;
	std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	return out;
}

std::string TrimWhitespace(std::string text) {
	const auto not_space = [](unsigned char ch) {
		return std::isspace(ch) == 0;
	};
	text.erase(text.begin(), std::find_if(text.begin(), text.end(), not_space));
	text.erase(std::find_if(text.rbegin(), text.rend(), not_space).base(), text.end());
	return text;
}

std::string QuoteForShell(const std::string& token) {
	if (token.empty()) {
		return "''";
	}
	bool needs_quote = false;
	for (char ch : token) {
		if (std::isspace(static_cast<unsigned char>(ch)) != 0 || ch == '\'' || ch == '"') {
			needs_quote = true;
			break;
		}
	}
	if (!needs_quote) {
		return token;
	}
	std::string out = "'";
	for (char ch : token) {
		if (ch == '\'') {
			out += "'\\''";
		} else {
			out.push_back(ch);
		}
	}
	out.push_back('\'');
	return out;
}

std::string BytesToHex(const uint8_t* bytes, size_t size) {
	static constexpr char kHex[] = "0123456789abcdef";
	std::string out;
	out.resize(size * 2u);
	for (size_t index = 0; index < size; ++index) {
		out[index * 2u] = kHex[(bytes[index] >> 4u) & 0x0Fu];
		out[index * 2u + 1u] = kHex[bytes[index] & 0x0Fu];
	}
	return out;
}

std::filesystem::path ResolveProjectRoot() {
	return ResolveSourceRoot().parent_path();
}

std::filesystem::path ResolveProjectRelativePath(const std::string& input) {
	if (input.empty()) {
		return ResolveProjectRoot();
	}
	std::filesystem::path path(input);
	if (path.is_absolute()) {
		return path.lexically_normal();
	}
	const std::filesystem::path current_relative = std::filesystem::current_path() / path;
	std::error_code current_error;
	if (std::filesystem::exists(current_relative, current_error)) {
		return current_relative.lexically_normal();
	}
	return (ResolveProjectRoot() / path).lexically_normal();
}

bool ReadAllBytes(const std::filesystem::path& path, std::vector<uint8_t>* bytes_out, std::string* error_out) {
	if (bytes_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "binary output target is null";
		}
		return false;
	}
	IO::AsyncIO::AsyncFileReader reader;
	if (!reader.OpenFile(path.string())) {
		if (error_out != nullptr) {
			*error_out = reader.GetLastError().empty() ? "failed to open file" : reader.GetLastError();
		}
		return false;
	}
	const int64_t file_size = reader.GetFileSize();
	if (file_size < 0) {
		if (error_out != nullptr) {
			*error_out = reader.GetLastError().empty() ? "failed to determine file size" : reader.GetLastError();
		}
		reader.CloseFile();
		return false;
	}
	std::vector<uint8_t> bytes;
	if (!reader.ReadSync(static_cast<size_t>(file_size), bytes)) {
		if (error_out != nullptr) {
			*error_out = reader.GetLastError().empty() ? "failed to read file" : reader.GetLastError();
		}
		reader.CloseFile();
		return false;
	}
	reader.CloseFile();
	*bytes_out = std::move(bytes);
	return true;
}

bool WriteAllBytes(const std::filesystem::path& path,
			   const std::vector<uint8_t>& bytes,
			   bool append,
			   std::string* error_out) {
	std::error_code dir_error;
	if (!path.parent_path().empty()) {
		std::filesystem::create_directories(path.parent_path(), dir_error);
	}
	IO::AsyncIO::AsyncFileWriter writer;
	if (!writer.CreateFile(path.string(), append)) {
		if (error_out != nullptr) {
			*error_out = writer.GetLastError().empty() ? "failed to open file for writing" : writer.GetLastError();
		}
		return false;
	}
	if (!bytes.empty() && !writer.WriteSync(bytes.data(), bytes.size())) {
		if (error_out != nullptr) {
			*error_out = writer.GetLastError().empty() ? "failed to write file" : writer.GetLastError();
		}
		writer.CloseFile();
		return false;
	}
	writer.CloseFile();
	return true;
}

bool GetObjectValue(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object> object,
			   const char* primary_key,
			   const char* secondary_key,
			   v8::Local<v8::Value>* value_out) {
	if (value_out == nullptr) {
		return false;
	}
	auto try_get = [&](const char* key) -> bool {
		if (key == nullptr) {
			return false;
		}
		return object->Get(context, Engine::Helper::ToV8Str(isolate, key)).ToLocal(value_out)
			&& !(*value_out).IsEmpty()
			&& !(*value_out)->IsUndefined();
	};
	return try_get(primary_key) || try_get(secondary_key);
}

bool GetObjectBool(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  v8::Local<v8::Object> object,
			  const char* primary_key,
			  const char* secondary_key,
			  bool fallback) {
	v8::Local<v8::Value> value;
	if (!GetObjectValue(isolate, context, object, primary_key, secondary_key, &value)) {
		return fallback;
	}
	return value->BooleanValue(isolate);
}

int GetObjectInt(v8::Isolate* isolate,
			 v8::Local<v8::Context> context,
			 v8::Local<v8::Object> object,
			 const char* primary_key,
			 const char* secondary_key,
			 int fallback) {
	v8::Local<v8::Value> value;
	if (!GetObjectValue(isolate, context, object, primary_key, secondary_key, &value) || !value->IsNumber()) {
		return fallback;
	}
	return value->Int32Value(context).FromMaybe(fallback);
}

std::vector<std::string> GetObjectStringArray(v8::Isolate* isolate,
				      v8::Local<v8::Context> context,
				      v8::Local<v8::Object> object,
				      const char* primary_key,
				      const char* secondary_key) {
	std::vector<std::string> values;
	v8::Local<v8::Value> value;
	if (!GetObjectValue(isolate, context, object, primary_key, secondary_key, &value) || !value->IsArray()) {
		return values;
	}
	v8::Local<v8::Array> array = value.As<v8::Array>();
	values.reserve(array->Length());
	for (uint32_t index = 0; index < array->Length(); ++index) {
		v8::Local<v8::Value> element;
		if (!array->Get(context, index).ToLocal(&element) || !element->IsString()) {
			continue;
		}
		values.push_back(Engine::Helper::FromV8Str(isolate, element));
	}
	return values;
}

std::vector<RuntimeLive::InterfaceCompiler::SourceFile> GetRuntimeSourceFiles(
						v8::Isolate* isolate,
						v8::Local<v8::Context> context,
						v8::Local<v8::Object> object) {
	std::vector<RuntimeLive::InterfaceCompiler::SourceFile> files;
	v8::Local<v8::Value> value;
	if (!GetObjectValue(isolate, context, object, "sourceFiles", "source_files", &value) || !value->IsArray()) {
		return files;
	}
	v8::Local<v8::Array> array = value.As<v8::Array>();
	for (uint32_t index = 0; index < array->Length(); ++index) {
		v8::Local<v8::Value> element;
		if (!array->Get(context, index).ToLocal(&element) || !element->IsObject()) {
			continue;
		}
		v8::Local<v8::Object> file_object = element.As<v8::Object>();
		RuntimeLive::InterfaceCompiler::SourceFile file;
		file.path = GetObjectString(isolate, context, file_object, "path", nullptr, std::string());
		file.content = GetObjectString(isolate, context, file_object, "content", nullptr, std::string());
		file.is_header = GetObjectBool(isolate, context, file_object, "isHeader", "is_header", false);
		if (!file.path.empty()) {
			files.push_back(std::move(file));
		}
	}
	return files;
}

struct CurlResponse {
	bool ok = false;
	long status_code = 0;
	double content_length = -1.0;
	std::string content_type;
	std::string effective_url;
	std::string body;
	std::string error;
};

size_t CurlWriteToString(void* data, size_t size, size_t nmemb, void* userdata) {
	const size_t bytes = size * nmemb;
	auto* output = static_cast<std::string*>(userdata);
	output->append(static_cast<const char*>(data), bytes);
	return bytes;
}

size_t CurlWriteToFile(void* data, size_t size, size_t nmemb, void* userdata) {
	const size_t bytes = size * nmemb;
	auto* output = static_cast<std::ofstream*>(userdata);
	output->write(static_cast<const char*>(data), static_cast<std::streamsize>(bytes));
	return bytes;
}

bool EnsureCurlInitialized() {
	static const CURLcode kInitCode = curl_global_init(CURL_GLOBAL_DEFAULT);
	return kInitCode == CURLE_OK;
}

bool PerformNetworkRequest(const std::string& url,
			       bool head_only,
			       const std::filesystem::path* output_path,
			       CurlResponse* response) {
	if (response == nullptr) {
		return false;
	}
	*response = CurlResponse();
	if (!EnsureCurlInitialized()) {
		response->error = "curl initialization failed";
		return false;
	}
	std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
	if (!curl) {
		response->error = "curl_easy_init failed";
		return false;
	}
	std::ofstream output_stream;
	char error_buffer[CURL_ERROR_SIZE] = {0};
	curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "EngineBuilder/1.1");
	curl_easy_setopt(curl.get(), CURLOPT_ERRORBUFFER, error_buffer);
	if (head_only) {
		curl_easy_setopt(curl.get(), CURLOPT_NOBODY, 1L);
	} else if (output_path != nullptr) {
		std::error_code dir_error;
		if (!output_path->parent_path().empty()) {
			std::filesystem::create_directories(output_path->parent_path(), dir_error);
		}
		output_stream.open(*output_path, std::ios::binary);
		if (!output_stream.is_open()) {
			response->error = "failed to open output path";
			return false;
		}
		curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, &CurlWriteToFile);
		curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &output_stream);
	} else {
		curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, &CurlWriteToString);
		curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &response->body);
	}
	const CURLcode code = curl_easy_perform(curl.get());
	if (output_stream.is_open()) {
		output_stream.close();
	}
	if (code != CURLE_OK) {
		response->error = error_buffer[0] != '\0' ? std::string(error_buffer) : curl_easy_strerror(code);
		return false;
	}
	char* content_type = nullptr;
	char* effective_url = nullptr;
	curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response->status_code);
	curl_easy_getinfo(curl.get(), CURLINFO_CONTENT_TYPE, &content_type);
	curl_easy_getinfo(curl.get(), CURLINFO_EFFECTIVE_URL, &effective_url);
	curl_easy_getinfo(curl.get(), CURLINFO_CONTENT_LENGTH_DOWNLOAD, &response->content_length);
	response->content_type = content_type != nullptr ? std::string(content_type) : std::string();
	response->effective_url = effective_url != nullptr ? std::string(effective_url) : std::string();
	response->ok = response->status_code >= 200 && response->status_code < 400;
	return true;
}

struct CommandResult {
	int exit_code = -1;
	std::string output;
	std::string error;
};

bool RunCommandCapture(const std::string& command, CommandResult* result) {
	if (result == nullptr) {
		return false;
	}
	*result = CommandResult();
	FILE* pipe = popen(command.c_str(), "r");
	if (pipe == nullptr) {
		result->error = "popen failed";
		return false;
	}
	std::array<char, 4096> buffer {};
	while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
		result->output += buffer.data();
	}
	const int status = pclose(pipe);
	if (status == -1) {
		result->error = "pclose failed";
		return false;
	}
	if (WIFEXITED(status)) {
		result->exit_code = WEXITSTATUS(status);
	} else {
		result->exit_code = status;
	}
	result->output = TrimWhitespace(result->output);
	return true;
}

std::string DetectPlatformName() {
#if defined(_WIN32)
	return "windows";
#elif defined(__APPLE__)
	return "macos";
#elif defined(__linux__)
	return "linux";
#elif defined(__OpenBSD__)
	return "openbsd";
#else
	return "unknown";
#endif
}

std::string DetectHostName() {
	std::array<char, 256> hostname {};
	if (gethostname(hostname.data(), hostname.size() - 1u) != 0) {
		return std::string();
	}
	hostname.back() = '\0';
	return hostname.data();
}

std::string WhichCommand(const std::string& command) {
	const char* path_env = std::getenv("PATH");
	if (path_env == nullptr || command.empty()) {
		return std::string();
	}
	std::stringstream stream(path_env);
	std::string segment;
	while (std::getline(stream, segment, ':')) {
		const std::filesystem::path candidate = std::filesystem::path(segment) / command;
		std::error_code error;
		if (std::filesystem::exists(candidate, error) && access(candidate.c_str(), X_OK) == 0) {
			return candidate.string();
		}
	}
	return std::string();
}

std::string Sha256HexString(const std::string& text) {
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(reinterpret_cast<const unsigned char*>(text.data()), text.size(), digest);
	return BytesToHex(digest, SHA256_DIGEST_LENGTH);
}

std::string Sha256HexFile(const std::filesystem::path& path, std::string* error_out) {
	std::vector<uint8_t> bytes;
	if (!ReadAllBytes(path, &bytes, error_out)) {
		return std::string();
	}
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(bytes.data(), bytes.size(), digest);
	return BytesToHex(digest, SHA256_DIGEST_LENGTH);
}

std::string Base64EncodeString(const std::string& text) {
	if (text.empty()) {
		return std::string();
	}
	std::string out(static_cast<size_t>(4 * ((text.size() + 2u) / 3u)), '\0');
	const int written = EVP_EncodeBlock(
		reinterpret_cast<unsigned char*>(out.data()),
		reinterpret_cast<const unsigned char*>(text.data()),
		static_cast<int>(text.size()));
	out.resize(written > 0 ? static_cast<size_t>(written) : 0u);
	return out;
}

bool Base64DecodeString(const std::string& encoded, std::string* decoded_out, std::string* error_out) {
	if (decoded_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "decoded output target is null";
		}
		return false;
	}
	if (encoded.empty()) {
		decoded_out->clear();
		return true;
	}
	std::vector<unsigned char> decoded(static_cast<size_t>(3 * (encoded.size() / 4u) + 3u), 0);
	int decoded_size = EVP_DecodeBlock(
		decoded.data(),
		reinterpret_cast<const unsigned char*>(encoded.data()),
		static_cast<int>(encoded.size()));
	if (decoded_size < 0) {
		if (error_out != nullptr) {
			*error_out = "invalid base64 input";
		}
		return false;
	}
	int padding = 0;
	if (!encoded.empty() && encoded.back() == '=') {
		++padding;
	}
	if (encoded.size() > 1u && encoded[encoded.size() - 2u] == '=') {
		++padding;
	}
	decoded_size -= padding;
	decoded_out->assign(reinterpret_cast<const char*>(decoded.data()), static_cast<size_t>(std::max(decoded_size, 0)));
	return true;
}

std::string RandomHex(size_t byte_count, std::string* error_out) {
	std::vector<uint8_t> bytes(byte_count, 0u);
	if (byte_count > 0u && RAND_bytes(bytes.data(), static_cast<int>(byte_count)) != 1) {
		if (error_out != nullptr) {
			*error_out = "RAND_bytes failed";
		}
		return std::string();
	}
	return BytesToHex(bytes.data(), bytes.size());
}

std::string ScanStatusToString(EngineDoctor::ScanStatus status) {
	switch (status) {
	case EngineDoctor::ScanStatus::PENDING:
		return "pending";
	case EngineDoctor::ScanStatus::SUCCESS:
		return "success";
	case EngineDoctor::ScanStatus::WARNING:
		return "warning";
	case EngineDoctor::ScanStatus::ERROR:
		return "error";
	case EngineDoctor::ScanStatus::SKIPPED:
		return "skipped";
	}
	return "unknown";
}

std::filesystem::path ResolveSourceRoot() {
	const char* env_root = std::getenv("ENGINE_MODULE_ROOT");
	if (env_root != nullptr && env_root[0] != '\0') {
		std::filesystem::path configured_root(env_root);
		std::error_code configured_error;
		if (std::filesystem::exists(configured_root, configured_error)) {
			if (configured_root.filename() == "src") {
				return configured_root.lexically_normal();
			}
			const std::filesystem::path nested_src = configured_root / "src";
			if (std::filesystem::exists(nested_src, configured_error)) {
				return nested_src.lexically_normal();
			}
		}
	}

	std::error_code current_error;
	const std::filesystem::path current = std::filesystem::current_path(current_error);
	if (!current_error) {
		if (current.filename() == "src") {
			return current.lexically_normal();
		}
		const std::filesystem::path nested_src = current / "src";
		std::error_code nested_error;
		if (std::filesystem::exists(nested_src, nested_error)) {
			return nested_src.lexically_normal();
		}
	}

	std::filesystem::path compiled_path = std::filesystem::path(__FILE__).parent_path().parent_path();
	if (compiled_path.filename() == "src") {
		return compiled_path.lexically_normal();
	}
	if (compiled_path.filename() == "modules") {
		return compiled_path.parent_path().lexically_normal();
	}
	return (compiled_path / "src").lexically_normal();
}

bool ResolveRelativeToSourceRoot(const std::string& requested,
					 std::filesystem::path* relative_out,
					 std::filesystem::path* absolute_out,
					 std::string* error_out) {
	if (relative_out == nullptr || absolute_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "source path output target is null";
		}
		return false;
	}

	std::filesystem::path relative = requested.empty()
		? std::filesystem::path()
		: std::filesystem::path(requested).lexically_normal();
	if (relative.is_absolute()) {
		if (error_out != nullptr) {
			*error_out = "source paths must be relative to src";
		}
		return false;
	}
	for (const auto& part : relative) {
		if (part == "..") {
			if (error_out != nullptr) {
				*error_out = "source path escapes src root";
			}
			return false;
		}
	}

	const std::filesystem::path source_root = ResolveSourceRoot();
	*relative_out = relative;
	*absolute_out = source_root / relative;
	return true;
}

template <typename T>
bool SetProperty(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object> object,
			   const char* name,
			   T value) {
	return object->Set(context, Engine::Helper::ToV8Str(isolate, name), value).FromMaybe(false);
}

v8::Local<v8::Array> MakeStringArray(v8::Isolate* isolate,
					 v8::Local<v8::Context> context,
					 const std::vector<std::string>& values) {
	v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(values.size()));
	for (size_t index = 0; index < values.size(); ++index) {
		array
			->Set(context,
			      static_cast<uint32_t>(index),
			      Engine::Helper::ToV8Str(isolate, values[index]))
			.FromMaybe(false);
	}
	return array;
}

bool RequireStringArg(const v8::FunctionCallbackInfo<v8::Value>& args,
			      int index,
			      const char* name,
			      std::string* out) {
	if (out == nullptr) {
		Engine::Helper::ThrowError(args.GetIsolate(), "string output target is null");
		return false;
	}
	if (args.Length() <= index || !args[index]->IsString()) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), name);
		return false;
	}
	*out = Engine::Helper::FromV8Str(args.GetIsolate(), args[index]);
	return true;
}

std::string OptionalStringArg(const v8::FunctionCallbackInfo<v8::Value>& args,
			     int index,
			     const std::string& fallback = std::string()) {
	if (args.Length() <= index || args[index]->IsUndefined() || args[index]->IsNull()) {
		return fallback;
	}
	if (!args[index]->IsString()) {
		return fallback;
	}
	return Engine::Helper::FromV8Str(args.GetIsolate(), args[index]);
}

std::string GetObjectString(v8::Isolate* isolate,
			    v8::Local<v8::Context> context,
			    v8::Local<v8::Object> object,
			    const char* primary_key,
			    const char* secondary_key,
			    const std::string& fallback) {
	auto get_value = [&](const char* key, std::string* out) -> bool {
		if (key == nullptr) {
			return false;
		}
		v8::Local<v8::Value> value;
		if (!object->Get(context, Engine::Helper::ToV8Str(isolate, key)).ToLocal(&value)) {
			return false;
		}
		if (!value->IsString()) {
			return false;
		}
		*out = Engine::Helper::FromV8Str(isolate, value);
		return true;
	};

	std::string out;
	if (get_value(primary_key, &out) || get_value(secondary_key, &out)) {
		return out;
	}
	return fallback;
}

std::vector<std::string> CollectRelativeFiles(const std::filesystem::path& absolute_root,
					      const std::filesystem::path& source_root) {
	std::vector<std::string> files;
	std::error_code error;
	if (!std::filesystem::exists(absolute_root, error)) {
		return files;
	}
	if (std::filesystem::is_regular_file(absolute_root, error)) {
		files.push_back(std::filesystem::relative(absolute_root, source_root, error).generic_string());
		return files;
	}
	std::filesystem::recursive_directory_iterator it(
		absolute_root,
		std::filesystem::directory_options::skip_permission_denied,
		error);
	std::filesystem::recursive_directory_iterator end;
	for (; it != end; it.increment(error)) {
		if (error) {
			error.clear();
			continue;
		}
		if (it->is_regular_file(error)) {
			files.push_back(std::filesystem::relative(it->path(), source_root, error).generic_string());
		}
	}
	std::sort(files.begin(), files.end());
	return files;
}

std::vector<std::string> CollectRelativeDirectories(const std::filesystem::path& absolute_root,
						    const std::filesystem::path& source_root) {
	std::vector<std::string> directories;
	std::error_code error;
	if (!std::filesystem::exists(absolute_root, error) || !std::filesystem::is_directory(absolute_root, error)) {
		return directories;
	}
	std::filesystem::recursive_directory_iterator it(
		absolute_root,
		std::filesystem::directory_options::skip_permission_denied,
		error);
	std::filesystem::recursive_directory_iterator end;
	for (; it != end; it.increment(error)) {
		if (error) {
			error.clear();
			continue;
		}
		if (it->is_directory(error)) {
			directories.push_back(std::filesystem::relative(it->path(), source_root, error).generic_string());
		}
	}
	std::sort(directories.begin(), directories.end());
	return directories;
}

void ContainerSourceRootCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), ResolveSourceRoot().string()));
}

void ContainerListSourceFilesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string error;
	std::filesystem::path relative;
	std::filesystem::path absolute;
	if (!ResolveRelativeToSourceRoot(OptionalStringArg(args, 0), &relative, &absolute, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	args.GetReturnValue().Set(MakeStringArray(isolate, context, CollectRelativeFiles(absolute, ResolveSourceRoot())));
}

void ContainerListSourceDirectoriesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string error;
	std::filesystem::path relative;
	std::filesystem::path absolute;
	if (!ResolveRelativeToSourceRoot(OptionalStringArg(args, 0), &relative, &absolute, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	args.GetReturnValue().Set(MakeStringArray(isolate, context, CollectRelativeDirectories(absolute, ResolveSourceRoot())));
}

void ContainerReadSourceFileCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	std::string requested;
	if (!RequireStringArg(args, 0, "readSourceFile expects src-relative path string", &requested)) {
		return;
	}
	std::string error;
	std::filesystem::path relative;
	std::filesystem::path absolute;
	if (!ResolveRelativeToSourceRoot(requested, &relative, &absolute, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	std::error_code stat_error;
	if (!std::filesystem::exists(absolute, stat_error) || !std::filesystem::is_regular_file(absolute, stat_error)) {
		Engine::Helper::ThrowError(isolate, "source file does not exist");
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(isolate, ToolTo::ReadTextFile(absolute)));
}

void ContainerFindSourceFilesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string query;
	if (!RequireStringArg(args, 0, "findSourceFiles expects search string", &query)) {
		return;
	}
	const std::string lowered_query = ToLowerCopy(query);
	std::vector<std::string> matches;
	for (const auto& file : CollectRelativeFiles(ResolveSourceRoot(), ResolveSourceRoot())) {
		if (ToLowerCopy(file).find(lowered_query) != std::string::npos) {
			matches.push_back(file);
		}
	}
	args.GetReturnValue().Set(MakeStringArray(isolate, context, matches));
}

void ContainerDescribeSourceTreeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string error;
	std::filesystem::path relative;
	std::filesystem::path absolute;
	if (!ResolveRelativeToSourceRoot(OptionalStringArg(args, 0), &relative, &absolute, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}

	const std::filesystem::path source_root = ResolveSourceRoot();
	const std::vector<std::string> files = CollectRelativeFiles(absolute, source_root);
	const std::vector<std::string> directories = CollectRelativeDirectories(absolute, source_root);
	std::vector<std::string> top_level_entries;
	std::error_code iter_error;
	if (std::filesystem::exists(absolute, iter_error) && std::filesystem::is_directory(absolute, iter_error)) {
		for (const auto& entry : std::filesystem::directory_iterator(absolute, std::filesystem::directory_options::skip_permission_denied, iter_error)) {
			if (iter_error) {
				iter_error.clear();
				continue;
			}
			top_level_entries.push_back(std::filesystem::relative(entry.path(), source_root, iter_error).generic_string());
		}
		std::sort(top_level_entries.begin(), top_level_entries.end());
	}

	v8::Local<v8::Object> result = v8::Object::New(isolate);
	SetProperty(isolate, context, result, "sourceRoot", Engine::Helper::ToV8Str(isolate, source_root.string()));
	SetProperty(isolate, context, result, "basePath", Engine::Helper::ToV8Str(isolate, relative.empty() ? std::string(".") : relative.generic_string()));
	SetProperty(isolate, context, result, "fileCount", v8::Number::New(isolate, static_cast<double>(files.size())));
	SetProperty(isolate, context, result, "directoryCount", v8::Number::New(isolate, static_cast<double>(directories.size())));
	SetProperty(isolate, context, result, "topLevelEntries", MakeStringArray(isolate, context, top_level_entries));
	args.GetReturnValue().Set(result);
}

void ContainerAvailableModulesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	args.GetReturnValue().Set(MakeStringArray(isolate, context, ListModules()));
}

void FileSystemExistsCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "exists expects path string", &path)) {
		return;
	}
	std::error_code error;
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), std::filesystem::exists(std::filesystem::path(path), error)));
}

void FileSystemIsFileCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "isFile expects path string", &path)) {
		return;
	}
	std::error_code error;
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), std::filesystem::is_regular_file(std::filesystem::path(path), error)));
}

void FileSystemIsDirectoryCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "isDirectory expects path string", &path)) {
		return;
	}
	std::error_code error;
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), std::filesystem::is_directory(std::filesystem::path(path), error)));
}

void FileSystemReadTextCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "readText expects path string", &path)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), ToolTo::ReadTextFile(std::filesystem::path(path))));
}

void FileSystemWriteTextCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	std::string path;
	std::string text;
	if (!RequireStringArg(args, 0, "writeText expects path string", &path)
		|| !RequireStringArg(args, 1, "writeText expects content string", &text)) {
		return;
	}
	std::error_code dir_error;
	const std::filesystem::path output_path(path);
	if (!output_path.parent_path().empty()) {
		std::filesystem::create_directories(output_path.parent_path(), dir_error);
	}
	std::string write_error;
	const bool ok = ToolTo::WriteTextFile(output_path, text, false, &write_error);
	if (!ok && !write_error.empty()) {
		Engine::Helper::ThrowError(isolate, write_error);
		return;
	}
	args.GetReturnValue().Set(v8::Boolean::New(isolate, ok));
}

void FileSystemCreateDirectoriesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "createDirectories expects path string", &path)) {
		return;
	}
	std::error_code error;
	const bool ok = std::filesystem::create_directories(std::filesystem::path(path), error)
		|| std::filesystem::exists(std::filesystem::path(path), error);
	args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ok && !error));
}

void FileSystemListDirCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string path = OptionalStringArg(args, 0, ".");
	std::vector<std::string> entries;
	std::error_code error;
	if (std::filesystem::exists(std::filesystem::path(path), error) && std::filesystem::is_directory(std::filesystem::path(path), error)) {
		for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(path), std::filesystem::directory_options::skip_permission_denied, error)) {
			if (error) {
				error.clear();
				continue;
			}
			entries.push_back(entry.path().filename().string());
		}
		std::sort(entries.begin(), entries.end());
	}
	args.GetReturnValue().Set(MakeStringArray(isolate, context, entries));
}

void UtilNormalizePathCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "normalizePath expects path string", &path)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), std::filesystem::path(path).lexically_normal().generic_string()));
}

void UtilJoinPathCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::filesystem::path joined;
	for (int index = 0; index < args.Length(); ++index) {
		if (!args[index]->IsString()) {
			Engine::Helper::ThrowTypeError(args.GetIsolate(), "joinPath expects string segments");
			return;
		}
		joined /= std::filesystem::path(Engine::Helper::FromV8Str(args.GetIsolate(), args[index]));
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), joined.lexically_normal().generic_string()));
}

void UtilBasenameCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "basename expects path string", &path)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), std::filesystem::path(path).filename().string()));
}

void UtilDirnameCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "dirname expects path string", &path)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), std::filesystem::path(path).parent_path().generic_string()));
}

void UtilExtensionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string path;
	if (!RequireStringArg(args, 0, "extension expects path string", &path)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), std::filesystem::path(path).extension().string()));
}

void UtilCurrentDirCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::error_code error;
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), std::filesystem::current_path(error).string()));
}

void UtilSourceRootCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), ResolveSourceRoot().string()));
}

Provider* UnwrapProviderStore(const v8::FunctionCallbackInfo<v8::Value>& args) {
	if (!args.This()->IsObject()) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "ProviderStore receiver is invalid");
		return nullptr;
	}
	Provider* provider = Engine::Helper::UnwrapPointer<Provider>(args.This().As<v8::Object>());
	if (provider == nullptr) {
		Engine::Helper::ThrowTypeError(args.GetIsolate(), "ProviderStore is not initialized");
	}
	return provider;
}

void PopulateProviderFromObject(v8::Isolate* isolate,
				v8::Local<v8::Context> context,
				Provider* provider,
				v8::Local<v8::Object> object) {
	v8::Local<v8::Array> keys;
	if (!object->GetOwnPropertyNames(context).ToLocal(&keys)) {
		return;
	}
	for (uint32_t index = 0; index < keys->Length(); ++index) {
		v8::Local<v8::Value> key;
		v8::Local<v8::Value> value;
		if (!keys->Get(context, index).ToLocal(&key) || !object->Get(context, key).ToLocal(&value)) {
			continue;
		}
		provider->set(Engine::Helper::FromV8Str(isolate, key), Engine::Helper::FromV8Str(isolate, value));
	}
}

v8::Local<v8::FunctionTemplate> MakeProviderStoreTemplate(v8::Isolate* isolate);

void ProviderStoreConstructor(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	if (!args.IsConstructCall()) {
		v8::Local<v8::Function> constructor = MakeProviderStoreTemplate(isolate)->GetFunction(context).ToLocalChecked();
		v8::Local<v8::Value> initial_value = args.Length() > 0
			? v8::Local<v8::Value>(args[0])
			: v8::Local<v8::Value>(v8::Undefined(isolate));
		v8::Local<v8::Value> argv[1] = {initial_value};
		v8::Local<v8::Object> instance = constructor->NewInstance(context, args.Length() > 0 ? 1 : 0, argv).ToLocalChecked();
		args.GetReturnValue().Set(instance);
		return;
	}

	auto* provider = new Provider();
	if (args.Length() > 0 && args[0]->IsObject()) {
		PopulateProviderFromObject(isolate, context, provider, args[0].As<v8::Object>());
	}
	Engine::Helper::WrapPointer(args.This(), provider);
	Engine::Helper::RegisterWeakCleanup(isolate, args.This(), provider);
	args.GetReturnValue().Set(args.This());
}

void ProviderStoreSetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Provider* provider = UnwrapProviderStore(args);
	if (provider == nullptr) {
		return;
	}
	std::string key;
	std::string value;
	if (!RequireStringArg(args, 0, "ProviderStore.set expects key string", &key)
		|| !RequireStringArg(args, 1, "ProviderStore.set expects value string", &value)) {
		return;
	}
	provider->set(std::move(key), std::move(value));
	args.GetReturnValue().Set(args.This());
}

void ProviderStoreGetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Provider* provider = UnwrapProviderStore(args);
	if (provider == nullptr) {
		return;
	}
	std::string key;
	if (!RequireStringArg(args, 0, "ProviderStore.get expects key string", &key)) {
		return;
	}
	const std::string* value = provider->get(key);
	if (value != nullptr) {
		args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), *value));
		return;
	}
	if (args.Length() > 1) {
		args.GetReturnValue().Set(args[1]);
		return;
	}
	args.GetReturnValue().Set(v8::Undefined(args.GetIsolate()));
}

void ProviderStoreKeysCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	Provider* provider = UnwrapProviderStore(args);
	if (provider == nullptr) {
		return;
	}
	args.GetReturnValue().Set(MakeStringArray(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), provider->keys()));
}

v8::Local<v8::FunctionTemplate> MakeProviderStoreTemplate(v8::Isolate* isolate) {
	return Engine::Helper::MakeClass(
		isolate,
		"ProviderStore",
		&ProviderStoreConstructor,
		{{"set", &ProviderStoreSetCallback},
		 {"get", &ProviderStoreGetCallback},
		 {"keys", &ProviderStoreKeysCallback}});
}

void ProviderCreateCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	v8::Local<v8::Function> constructor = MakeProviderStoreTemplate(isolate)->GetFunction(context).ToLocalChecked();
	v8::Local<v8::Value> initial_value = args.Length() > 0
		? v8::Local<v8::Value>(args[0])
		: v8::Local<v8::Value>(v8::Undefined(isolate));
	v8::Local<v8::Value> argv[1] = {initial_value};
	v8::Local<v8::Object> instance = constructor->NewInstance(context, args.Length() > 0 ? 1 : 0, argv).ToLocalChecked();
	args.GetReturnValue().Set(instance);
}

RuntimeLive::InterfaceCompilerOptions ParseRuntimeOptions(v8::Isolate* isolate,
					 v8::Local<v8::Context> context,
					 const v8::FunctionCallbackInfo<v8::Value>& args,
					 int index) {
	RuntimeLive::InterfaceCompilerOptions options;
	options.cache_dir = (std::filesystem::current_path() / "out/runtime_live_module_cache").string();
	options.provider = RuntimeLive::kProviderGPUToolkit;
	options.language = "cpp";
	if (args.Length() <= index || !args[index]->IsObject()) {
		return options;
	}
	v8::Local<v8::Object> object = args[index].As<v8::Object>();
	options.cache_dir = GetObjectString(isolate, context, object, "cacheDir", "cache_dir", options.cache_dir);
	options.provider = GetObjectString(isolate, context, object, "provider", nullptr, options.provider);
	options.language = GetObjectString(isolate, context, object, "language", nullptr, options.language);
	options.compile_flags = GetObjectString(isolate, context, object, "compileFlags", "compile_flags", "");
	options.link_flags = GetObjectString(isolate, context, object, "linkFlags", "link_flags", "");
	return options;
}

CompilerSource BuildCompilerSourceFromArgs(v8::Isolate* isolate,
					   v8::Local<v8::Context> context,
					   const v8::FunctionCallbackInfo<v8::Value>& args,
					   std::string* source_out) {
	std::string source = source_out == nullptr ? std::string() : *source_out;
	if (source_out != nullptr) {
		source = *source_out;
	}
	const RuntimeLive::InterfaceCompilerOptions options = ParseRuntimeOptions(isolate, context, args, 1);
	return CompilerSource(source,
			      options.cache_dir,
			      options.provider,
			      options.language,
			      options.compile_flags,
			      options.link_flags);
}

void RuntimeLiveDescribeCompilerCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string source;
	if (!RequireStringArg(args, 0, "describeCompiler expects source string", &source)) {
		return;
	}
	const RuntimeLive::InterfaceCompilerOptions options = ParseRuntimeOptions(isolate, context, args, 1);
	const CompilerSource compiler_source(source,
					 options.cache_dir,
					 options.provider,
					 options.language,
					 options.compile_flags,
					 options.link_flags);
	v8::Local<v8::Object> description = v8::Object::New(isolate);
	SetProperty(isolate, context, description, "cacheDir", Engine::Helper::ToV8Str(isolate, compiler_source.cache_dir().string()));
	SetProperty(isolate, context, description, "sourcePath", Engine::Helper::ToV8Str(isolate, compiler_source.source_path().string()));
	SetProperty(isolate, context, description, "binaryPath", Engine::Helper::ToV8Str(isolate, compiler_source.binary_path().string()));
	SetProperty(isolate, context, description, "compileOutPath", Engine::Helper::ToV8Str(isolate, compiler_source.compile_out_path().string()));
	SetProperty(isolate, context, description, "compileErrPath", Engine::Helper::ToV8Str(isolate, compiler_source.compile_err_path().string()));
	SetProperty(isolate, context, description, "runOutPath", Engine::Helper::ToV8Str(isolate, compiler_source.run_out_path().string()));
	SetProperty(isolate, context, description, "runErrPath", Engine::Helper::ToV8Str(isolate, compiler_source.run_err_path().string()));
	SetProperty(isolate, context, description, "useCpp", v8::Boolean::New(isolate, compiler_source.use_cpp()));
	SetProperty(isolate, context, description, "compilerArgs", MakeStringArray(isolate, context, compiler_source.BuildCompilerArgs()));
	args.GetReturnValue().Set(description);
}

void RuntimeLiveBuildCommandPreviewCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string source;
	if (!RequireStringArg(args, 0, "buildCompilerCommandPreview expects source string", &source)) {
		return;
	}
	const RuntimeLive::InterfaceCompilerOptions options = ParseRuntimeOptions(isolate, context, args, 1);
	const std::string compiler_binary = OptionalStringArg(args, 2, options.language == "c" ? "clang" : "clang++");
	const CompilerSource compiler_source(source,
					 options.cache_dir,
					 options.provider,
					 options.language,
					 options.compile_flags,
					 options.link_flags);
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(isolate, compiler_source.BuildCompilerCommandPreview(compiler_binary)));
}

void RuntimeLiveDefaultProviderCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), RuntimeLive::kProviderGPUToolkit));
}

bool BuildContainerModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  v8::Local<v8::Object>* module_out,
			  std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "sourceRoot", &ContainerSourceRootCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "listSourceFiles", &ContainerListSourceFilesCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "listSourceDirectories", &ContainerListSourceDirectoriesCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "readSourceFile", &ContainerReadSourceFileCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "findSourceFiles", &ContainerFindSourceFilesCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "describeSourceTree", &ContainerDescribeSourceTreeCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "availableModules", &ContainerAvailableModulesCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Container module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

bool BuildFileSystemModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "exists", &FileSystemExistsCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "isFile", &FileSystemIsFileCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "isDirectory", &FileSystemIsDirectoryCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "readText", &FileSystemReadTextCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "writeText", &FileSystemWriteTextCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "createDirectories", &FileSystemCreateDirectoriesCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "listDir", &FileSystemListDirCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build FileSystem module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

bool BuildUtilModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  v8::Local<v8::Object>* module_out,
			  std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "normalizePath", &UtilNormalizePathCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "joinPath", &UtilJoinPathCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "basename", &UtilBasenameCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "dirname", &UtilDirnameCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "extension", &UtilExtensionCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "cwd", &UtilCurrentDirCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "sourceRoot", &UtilSourceRootCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Util module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

bool BuildProviderModule(v8::Isolate* isolate,
			      v8::Local<v8::Context> context,
			      v8::Local<v8::Object>* module_out,
			      std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	v8::Local<v8::Function> provider_store = MakeProviderStoreTemplate(isolate)->GetFunction(context).ToLocalChecked();
	bool ok = true;
	ok = ok && SetProperty(isolate, context, module, "ProviderStore", provider_store);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "create", &ProviderCreateCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Provider module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

bool BuildRuntimeLiveModule(v8::Isolate* isolate,
			     v8::Local<v8::Context> context,
			     v8::Local<v8::Object>* module_out,
			     std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && SetProperty(isolate, context, module, "gpuToolkitProvider", Engine::Helper::ToV8Str(isolate, RuntimeLive::kProviderGPUToolkit));
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "defaultProvider", &RuntimeLiveDefaultProviderCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "describeCompiler", &RuntimeLiveDescribeCompilerCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "buildCompilerCommandPreview", &RuntimeLiveBuildCommandPreviewCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build RuntimeLive module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace

std::string ResolveCanonicalModuleName(const std::string& module_name) {
	const std::string normalized = NormalizeModuleKey(module_name);
	if (normalized == "container" || normalized == "project" || normalized == "sourcetree") {
		return "Container";
	}
	if (normalized == "filesystem" || normalized == "fs") {
		return "FileSystem";
	}
	if (normalized == "util" || normalized == "utils") {
		return "Util";
	}
	if (normalized == "provider") {
		return "Provider";
	}
	if (normalized == "runtimelive" || normalized == "live") {
		return "RuntimeLive";
	}
	return std::string();
}

std::vector<std::string> ListModules() {
	return {"Container", "FileSystem", "Provider", "RuntimeLive", "Util"};
}

bool ImportModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  const std::string& module_name,
			  std::string* error_out) {
	const std::string canonical_name = ResolveCanonicalModuleName(module_name);
	if (canonical_name.empty()) {
		if (error_out != nullptr) {
			*error_out = "unknown module: " + module_name;
		}
		return false;
	}

	v8::Local<v8::Object> module;
	bool ok = false;
	if (canonical_name == "Container") {
		ok = BuildContainerModule(isolate, context, &module, error_out);
	} else if (canonical_name == "FileSystem") {
		ok = BuildFileSystemModule(isolate, context, &module, error_out);
	} else if (canonical_name == "Util") {
		ok = BuildUtilModule(isolate, context, &module, error_out);
	} else if (canonical_name == "Provider") {
		ok = BuildProviderModule(isolate, context, &module, error_out);
	} else if (canonical_name == "RuntimeLive") {
		ok = BuildRuntimeLiveModule(isolate, context, &module, error_out);
	}
	if (!ok) {
		if (error_out != nullptr && error_out->empty()) {
			*error_out = "failed to initialize module: " + canonical_name;
		}
		return false;
	}

	if (!Engine::Helper::ExportGlobalModule(isolate, context, canonical_name.c_str(), module)) {
		if (error_out != nullptr) {
			*error_out = "failed to export module to global scope: " + canonical_name;
		}
		return false;
	}
	return true;
}

}  // namespace modules