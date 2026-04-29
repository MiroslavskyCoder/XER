#include "modules/module_common.h"

#include "async_io/async_file_reader.h"
#include "async_io/async_file_writer.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace modules::detail {

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

bool ReadAllBytes(const std::filesystem::path& path,
			 std::vector<uint8_t>* bytes_out,
			 std::string* error_out) {
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

v8::Local<v8::Array> MakeStringArray(v8::Isolate* isolate,
				     v8::Local<v8::Context> context,
				     const std::vector<std::string>& values) {
	v8::Local<v8::Array> array = v8::Array::New(isolate, static_cast<int>(values.size()));
	for (size_t index = 0; index < values.size(); ++index) {
		array->Set(context, static_cast<uint32_t>(index), Engine::Helper::ToV8Str(isolate, values[index])).FromMaybe(false);
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
			      const std::string& fallback) {
	if (args.Length() <= index || args[index]->IsUndefined() || args[index]->IsNull()) {
		return fallback;
	}
	if (!args[index]->IsString()) {
		return fallback;
	}
	return Engine::Helper::FromV8Str(args.GetIsolate(), args[index]);
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

}  // namespace modules::detail