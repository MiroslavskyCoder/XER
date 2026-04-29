#include "modules/module_builders.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace modules::detail {
namespace {

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

void CryptoSha256Callback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string text;
	if (!RequireStringArg(args, 0, "sha256 expects input string", &text)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), Sha256HexString(text)));
}

void CryptoSha256FileCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	std::string path;
	if (!RequireStringArg(args, 0, "sha256File expects path string", &path)) {
		return;
	}
	std::string error;
	const std::string hash = Sha256HexFile(std::filesystem::path(path), &error);
	if (!error.empty() && hash.empty()) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(isolate, hash));
}

void CryptoRandomHexCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	const size_t byte_count = args.Length() > 0 && args[0]->IsNumber()
		? static_cast<size_t>(std::max(0, args[0]->Int32Value(isolate->GetCurrentContext()).FromMaybe(32)))
		: 32u;
	std::string error;
	const std::string value = RandomHex(byte_count, &error);
	if (!error.empty() && value.empty()) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(isolate, value));
}

void CryptoBase64EncodeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	std::string text;
	if (!RequireStringArg(args, 0, "base64Encode expects input string", &text)) {
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(args.GetIsolate(), Base64EncodeString(text)));
}

void CryptoBase64DecodeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = args.GetIsolate();
	std::string encoded;
	if (!RequireStringArg(args, 0, "base64Decode expects input string", &encoded)) {
		return;
	}
	std::string decoded;
	std::string error;
	if (!Base64DecodeString(encoded, &decoded, &error)) {
		Engine::Helper::ThrowError(isolate, error);
		return;
	}
	args.GetReturnValue().Set(Engine::Helper::ToV8Str(isolate, decoded));
}

}  // namespace

bool BuildCryptoModule(v8::Isolate* isolate,
			   v8::Local<v8::Context> context,
			   v8::Local<v8::Object>* module_out,
			   std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = true;
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "sha256", &CryptoSha256Callback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "sha256File", &CryptoSha256FileCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "randomHex", &CryptoRandomHexCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "base64Encode", &CryptoBase64EncodeCallback);
	ok = ok && Engine::Helper::SetMethod(isolate, context, module, "base64Decode", &CryptoBase64DecodeCallback);
	if (!ok) {
		if (error_out != nullptr) {
			*error_out = "failed to build Crypto module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail