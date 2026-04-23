#include "modules/network_module.h"

#include <arpa/inet.h>
#include <curl/curl.h>
#include <netdb.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <utility>
#include <string>
#include <vector>

namespace {

struct HttpResponseData {
    std::string method;
    std::string requested_url;
    std::string effective_url;
    std::string body;
    std::string raw_headers;
    std::string content_type;
    std::string error;
    std::vector<std::pair<std::string, std::string>> headers;
    long status_code = 0;
    long redirect_count = 0;
    double total_time_ms = 0.0;
    bool ok = false;
};

std::string ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return *utf8;
}

std::vector<std::string> ResolveHostImpl(const std::string& host) {
    std::vector<std::string> addresses;

    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &result) != 0) {
        return addresses;
    }

    for (addrinfo* it = result; it != nullptr; it = it->ai_next) {
        char ip[INET6_ADDRSTRLEN] = {0};
        void* addr_ptr = nullptr;

        if (it->ai_family == AF_INET) {
            addr_ptr = &reinterpret_cast<sockaddr_in*>(it->ai_addr)->sin_addr;
        } else if (it->ai_family == AF_INET6) {
            addr_ptr = &reinterpret_cast<sockaddr_in6*>(it->ai_addr)->sin6_addr;
        } else {
            continue;
        }

        if (inet_ntop(it->ai_family, addr_ptr, ip, sizeof(ip)) != nullptr) {
            addresses.emplace_back(ip);
        }
    }

    freeaddrinfo(result);
    return addresses;
}

v8::Local<v8::String> ToV8String(v8::Isolate* isolate, const std::string& value) {
    return v8::String::NewFromUtf8(isolate,
                                   value.data(),
                                   v8::NewStringType::kNormal,
                                   static_cast<int>(value.size()))
        .ToLocalChecked();
}

std::string TrimString(const std::string& value) {
    const auto not_space = [](unsigned char c) { return !std::isspace(c); };
    auto begin = std::find_if(value.begin(), value.end(), not_space);
    auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    if (begin >= end) {
        return std::string();
    }
    return std::string(begin, end);
}

size_t CurlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    if (userdata == nullptr) {
        return 0;
    }

    std::string* out = reinterpret_cast<std::string*>(userdata);
    out->append(ptr, size * nmemb);
    return size * nmemb;
}

size_t CurlHeaderCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    const size_t total = size * nmemb;
    if (userdata == nullptr || ptr == nullptr || total == 0) {
        return total;
    }

    HttpResponseData* out = reinterpret_cast<HttpResponseData*>(userdata);
    std::string line(ptr, total);
    out->raw_headers.append(line);

    const std::string trimmed = TrimString(line);
    const size_t sep = trimmed.find(':');
    if (sep != std::string::npos) {
        std::string key = TrimString(trimmed.substr(0, sep));
        std::string value = TrimString(trimmed.substr(sep + 1));
        if (!key.empty()) {
            out->headers.emplace_back(std::move(key), std::move(value));
        }
    }

    return total;
}

HttpResponseData CurlRequest(const std::string& method,
                             const std::string& url,
                             const std::string& body) {
    static const bool curl_initialized = (curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
    HttpResponseData out;
    out.method = method;
    out.requested_url = url;

    if (!curl_initialized) {
        out.error = "curl_global_init failed";
        return out;
    }

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        out.error = "curl_easy_init failed";
        return out;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out.body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CurlHeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &out);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "EngineBuilder/1.0");
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    } else if (method == "PUT") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    } else {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

    const CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        out.error = curl_easy_strerror(rc);
        out.body.clear();
    } else {
        out.ok = true;
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &out.status_code);
    curl_easy_getinfo(curl, CURLINFO_REDIRECT_COUNT, &out.redirect_count);

    double total_time_sec = 0.0;
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time_sec);
    out.total_time_ms = total_time_sec * 1000.0;

    char* effective_url = nullptr;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);
    if (effective_url != nullptr) {
        out.effective_url = effective_url;
    }

    char* content_type = nullptr;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
    if (content_type != nullptr) {
        out.content_type = content_type;
    }

    curl_easy_cleanup(curl);
    return out;
}

std::string BuildHttpUrl(const std::string& host, const std::string& path, int port) {
    std::string normalized_path = path;
    if (normalized_path.empty()) {
        normalized_path = "/";
    } else if (normalized_path.front() != '/') {
        normalized_path = "/" + normalized_path;
    }

    return "http://" + host + ":" + std::to_string(port) + normalized_path;
}

HttpResponseData HttpGetImpl(const std::string& host, const std::string& path, int port) {
    const std::string url = BuildHttpUrl(host, path, port);
    return CurlRequest("GET", url, std::string());
}

v8::Local<v8::Object> ToV8Response(v8::Isolate* isolate,
                                   v8::Local<v8::Context> context,
                                   const HttpResponseData& response) {
    v8::Local<v8::Object> out = v8::Object::New(isolate);
    (void)out->Set(context,
                   v8::String::NewFromUtf8Literal(isolate, "ok"),
                   v8::Boolean::New(isolate, response.ok)).FromMaybe(false);
    (void)out->Set(context,
                   v8::String::NewFromUtf8Literal(isolate, "method"),
                   ToV8String(isolate, response.method)).FromMaybe(false);
    (void)out->Set(context,
                   v8::String::NewFromUtf8Literal(isolate, "requestedUrl"),
                   ToV8String(isolate, response.requested_url)).FromMaybe(false);
    (void)out->Set(context,
                   v8::String::NewFromUtf8Literal(isolate, "url"),
                   ToV8String(isolate, response.effective_url.empty() ? response.requested_url
                                                                      : response.effective_url))
        .FromMaybe(false);
    (void)out->Set(context,
                   v8::String::NewFromUtf8Literal(isolate, "statusCode"),
                   v8::Integer::New(isolate, static_cast<int>(response.status_code))).FromMaybe(false);
    (void)out->Set(context,
                   v8::String::NewFromUtf8Literal(isolate, "contentType"),
                   ToV8String(isolate, response.content_type)).FromMaybe(false);
    (void)out->Set(context,
                   v8::String::NewFromUtf8Literal(isolate, "totalTimeMs"),
                   v8::Number::New(isolate, response.total_time_ms)).FromMaybe(false);
    (void)out->Set(context,
                   v8::String::NewFromUtf8Literal(isolate, "redirectCount"),
                   v8::Integer::New(isolate, static_cast<int>(response.redirect_count))).FromMaybe(false);
    (void)out->Set(context,
                   v8::String::NewFromUtf8Literal(isolate, "rawHeaders"),
                   ToV8String(isolate, response.raw_headers)).FromMaybe(false);
    (void)out->Set(context,
                   v8::String::NewFromUtf8Literal(isolate, "body"),
                   ToV8String(isolate, response.body)).FromMaybe(false);
    (void)out->Set(context,
                   v8::String::NewFromUtf8Literal(isolate, "bodySize"),
                   v8::Integer::New(isolate, static_cast<int>(response.body.size()))).FromMaybe(false);
    (void)out->Set(context,
                   v8::String::NewFromUtf8Literal(isolate, "error"),
                   ToV8String(isolate, response.error)).FromMaybe(false);

    v8::Local<v8::Object> headers = v8::Object::New(isolate);
    for (const auto& [key, value] : response.headers) {
        (void)headers->Set(context, ToV8String(isolate, key), ToV8String(isolate, value)).FromMaybe(false);
    }
    (void)out->Set(context,
                   v8::String::NewFromUtf8Literal(isolate, "headers"),
                   headers).FromMaybe(false);

    return out;
}

void ResolveHostCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        args.GetReturnValue().Set(v8::Array::New(isolate));
        return;
    }

    const std::vector<std::string> addresses = ResolveHostImpl(ValueToString(isolate, args[0]));
    v8::Local<v8::Array> out = v8::Array::New(isolate, static_cast<int>(addresses.size()));
    for (uint32_t i = 0; i < addresses.size(); ++i) {
        (void)out
            ->Set(context,
                  i,
                  v8::String::NewFromUtf8(isolate, addresses[i].c_str()).ToLocalChecked())
            .FromMaybe(false);
    }

    args.GetReturnValue().Set(out);
}

void HttpGetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "httpGet expects host string")));
        return;
    }

    const std::string host = ValueToString(isolate, args[0]);
    std::string path = "/";
    int port = 80;

    if (args.Length() > 1 && args[1]->IsString()) {
        path = ValueToString(isolate, args[1]);
        if (path.empty()) {
            path = "/";
        }
    }

    if (args.Length() > 2 && args[2]->IsNumber()) {
        port = args[2].As<v8::Number>()->Value();
    }

    const HttpResponseData response = HttpGetImpl(host, path, port);
    args.GetReturnValue().Set(ToV8Response(isolate, context, response));
}

void HttpMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args, const std::string& method) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    if (args.Length() < 1 || !args[0]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "request expects url string")));
        return;
    }

    const std::string url = ValueToString(isolate, args[0]);
    std::string body;
    if (args.Length() > 1 && args[1]->IsString()) {
        body = ValueToString(isolate, args[1]);
    }

    const HttpResponseData response = CurlRequest(method, url, body);
    args.GetReturnValue().Set(ToV8Response(isolate, context, response));
}

void GetCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    HttpMethodCallback(args, "GET");
}

void PostCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    HttpMethodCallback(args, "POST");
}

void PutCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    HttpMethodCallback(args, "PUT");
}

}  // namespace

namespace modules {

bool RegisterNetworkModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> network = v8::Object::New(isolate);
    bool ok = network
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "resolveHost"),
                        v8::Function::New(context, ResolveHostCallback).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && network
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "httpGet"),
                         v8::Function::New(context, HttpGetCallback).ToLocalChecked())
                   .FromMaybe(false);
        ok = ok && network
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "get"),
                     v8::Function::New(context, GetCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && network
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "post"),
                     v8::Function::New(context, PostCallback).ToLocalChecked())
                 .FromMaybe(false);
        ok = ok && network
                 ->Set(context,
                     v8::String::NewFromUtf8Literal(isolate, "put"),
                     v8::Function::New(context, PutCallback).ToLocalChecked())
                 .FromMaybe(false);

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Network"), network)
        .FromMaybe(false);
}

}  // namespace modules
