#include "modules/module_builders.h"

#include <curl/curl.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace modules::detail {
namespace {

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

v8::Local<v8::Object> BuildCurlResponseObject(v8::Isolate* isolate,
				      v8::Local<v8::Context> context,
				      const CurlResponse& response) {
	v8::Local<v8::Object> result = v8::Object::New(isolate);
	SetProperty(isolate, context, result, "ok", v8::Boolean::New(isolate, response.ok));
	SetProperty(isolate, context, result, "statusCode", v8::Number::New(isolate, static_cast<double>(response.status_code)));
	SetProperty(isolate, context, result, "contentType", Engine::Helper::ToV8Str(isolate, response.content_type));
	SetProperty(isolate, context, result, "effectiveUrl", Engine::Helper::ToV8Str(isolate, response.effective_url));
	SetProperty(isolate, context, result, "contentLength", v8::Number::New(isolate, response.content_length));
	SetProperty(isolate, context, result, "body", Engine::Helper::ToV8Str(isolate, response.body));
	SetProperty(isolate, context, result, "error", Engine::Helper::ToV8Str(isolate, response.error));
	return result;
}

void NetworkFetchTextCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string url;
	if (!RequireStringArg(args, 0, "fetchText expects url string", &url)) {
		return;
	}
	CurlResponse response;
	if (!PerformNetworkRequest(url, false, nullptr, &response)) {
		Engine::Helper::ThrowError(isolate, response.error.empty() ? "network request failed" : response.error);
		return;
	}
	if (!response.ok) {
		args.GetReturnValue().Set(BuildCurlResponseObject(isolate, context, response));
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(isolate, response.body));
}

void NetworkProbeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string url;
	if (!RequireStringArg(args, 0, "probe expects url string", &url)) {
		return;
	}
	CurlResponse response;
	if (!PerformNetworkRequest(url, true, nullptr, &response)) {
		Engine::Helper::ThrowError(isolate, response.error.empty() ? "network probe failed" : response.error);
		return;
	}
	args.GetReturnValue().Set(BuildCurlResponseObject(isolate, context, response));
}

void NetworkDownloadCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	v8::Local<v8::Context> context = isolate->GetCurrentContext();
	std::string url;
	std::string output_path;
	if (!RequireStringArg(args, 0, "download expects url string", &url)
		|| !RequireStringArg(args, 1, "download expects output path string", &output_path)) {
		return;
	}
	CurlResponse response;
	const std::filesystem::path path(output_path);
	if (!PerformNetworkRequest(url, false, &path, &response)) {
		Engine::Helper::ThrowError(isolate, response.error.empty() ? "download failed" : response.error);
		return;
	}
	v8::Local<v8::Object> result = BuildCurlResponseObject(isolate, context, response);
	SetProperty(isolate, context, result, "outputPath", Engine::Helper::ToV8Str(isolate, path.string()));
	args.GetReturnValue().Set(result);
}

}  // namespace

bool BuildNetworkModule(v8::Isolate* isolate,
			    v8::Local<v8::Context> context,
			    v8::Local<v8::Object>* module_out,
			    std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "fetchText", &NetworkFetchTextCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "probe", &NetworkProbeCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "download", &NetworkDownloadCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Network module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail