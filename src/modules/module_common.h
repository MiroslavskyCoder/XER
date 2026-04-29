#pragma once

#include "modules/module_registry.h"

#include "helper/class_builder.h"
#include "helper/module_builder.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <v8.h>

namespace modules::detail {

template <typename T>
bool SetProperty(v8::Isolate* isolate,
			 v8::Local<v8::Context> context,
			 v8::Local<v8::Object> object,
			 const char* name,
			 T value) {
	return object->Set(context, Engine::Helper::ToV8Str(isolate, name), value).FromMaybe(false);
}

std::string ToLowerCopy(const std::string& text);
std::string TrimWhitespace(std::string text);
std::string QuoteForShell(const std::string& token);

std::filesystem::path ResolveSourceRoot();
std::filesystem::path ResolveProjectRoot();
std::filesystem::path ResolveProjectRelativePath(const std::string& input);

bool ResolveRelativeToSourceRoot(const std::string& requested,
					std::filesystem::path* relative_out,
					std::filesystem::path* absolute_out,
					std::string* error_out);

bool ReadAllBytes(const std::filesystem::path& path,
			 std::vector<uint8_t>* bytes_out,
			 std::string* error_out);
bool WriteAllBytes(const std::filesystem::path& path,
			  const std::vector<uint8_t>& bytes,
			  bool append,
			  std::string* error_out);

bool GetObjectValue(v8::Isolate* isolate,
			 v8::Local<v8::Context> context,
			 v8::Local<v8::Object> object,
			 const char* primary_key,
			 const char* secondary_key,
			 v8::Local<v8::Value>* value_out);
bool GetObjectBool(v8::Isolate* isolate,
			v8::Local<v8::Context> context,
			v8::Local<v8::Object> object,
			const char* primary_key,
			const char* secondary_key,
			bool fallback);
std::string GetObjectString(v8::Isolate* isolate,
				    v8::Local<v8::Context> context,
				    v8::Local<v8::Object> object,
				    const char* primary_key,
				    const char* secondary_key,
				    const std::string& fallback);
std::vector<std::string> GetObjectStringArray(v8::Isolate* isolate,
					      v8::Local<v8::Context> context,
					      v8::Local<v8::Object> object,
					      const char* primary_key,
					      const char* secondary_key);

v8::Local<v8::Array> MakeStringArray(v8::Isolate* isolate,
				     v8::Local<v8::Context> context,
				     const std::vector<std::string>& values);

bool RequireStringArg(const v8::FunctionCallbackInfo<v8::Value>& args,
			      int index,
			      const char* name,
			      std::string* out);
std::string OptionalStringArg(const v8::FunctionCallbackInfo<v8::Value>& args,
			      int index,
			      const std::string& fallback = std::string());

std::vector<std::string> CollectRelativeFiles(const std::filesystem::path& absolute_root,
					      const std::filesystem::path& source_root);
std::vector<std::string> CollectRelativeDirectories(const std::filesystem::path& absolute_root,
					    const std::filesystem::path& source_root);

}  // namespace modules::detail