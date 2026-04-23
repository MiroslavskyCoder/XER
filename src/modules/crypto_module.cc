#include "modules/crypto_module.h"

#include <iomanip>
#include <openssl/evp.h>
#include <openssl/rand.h>
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

std::string BytesToHex(const std::vector<uint8_t>& bytes) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (uint8_t b : bytes) {
        out << std::setw(2) << static_cast<int>(b);
    }
    return out.str();
}

std::vector<uint8_t> GenerateRandomBytes(size_t count) {
    std::vector<uint8_t> bytes;
    if (count == 0) {
        return bytes;
    }

    bytes.resize(count);
    if (RAND_bytes(bytes.data(), static_cast<int>(count)) != 1) {
        bytes.clear();
    }
    return bytes;
}

std::string Base64Encode(const std::string& input) {
    static const char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string output;
    output.reserve(((input.size() + 2) / 3) * 4);

    int val = 0;
    int valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            output.push_back(kAlphabet[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6) {
        output.push_back(kAlphabet[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (output.size() % 4 != 0) {
        output.push_back('=');
    }

    return output;
}

int Base64DecodeChar(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }
    if (c == '+') {
        return 62;
    }
    if (c == '/') {
        return 63;
    }
    return -1;
}

std::string Base64Decode(const std::string& input) {
    std::string output;
    int val = 0;
    int valb = -8;
    for (unsigned char c : input) {
        if (c == '=') {
            break;
        }
        int d = Base64DecodeChar(static_cast<char>(c));
        if (d == -1) {
            continue;
        }
        val = (val << 6) + d;
        valb += 6;
        if (valb >= 0) {
            output.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return output;
}

std::string HashDigestHex(const std::string& input, const std::string& algorithm) {
    const EVP_MD* md = EVP_get_digestbyname(algorithm.c_str());
    if (md == nullptr) {
        return std::string();
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        return std::string();
    }

    std::vector<uint8_t> digest(static_cast<size_t>(EVP_MD_get_size(md)));
    unsigned int digest_len = 0;
    bool ok = EVP_DigestInit_ex(ctx, md, nullptr) == 1 &&
              EVP_DigestUpdate(ctx, input.data(), input.size()) == 1 &&
              EVP_DigestFinal_ex(ctx, digest.data(), &digest_len) == 1;
    EVP_MD_CTX_free(ctx);

    if (!ok) {
        return std::string();
    }

    digest.resize(digest_len);
    return BytesToHex(digest);
}

void RandomBytesCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    int count = 16;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        count = args[0].As<v8::Number>()->Value();
    }
    if (count < 0) {
        count = 0;
    }
    if (count > 1024 * 1024) {
        count = 1024 * 1024;
    }

    const std::vector<uint8_t> bytes = GenerateRandomBytes(static_cast<size_t>(count));
    v8::Local<v8::Array> result = v8::Array::New(isolate, count);
    for (int i = 0; i < count; ++i) {
        (void)result->Set(context, i, v8::Integer::New(isolate, bytes[static_cast<size_t>(i)])).FromMaybe(false);
    }

    args.GetReturnValue().Set(result);
}

void RandomHexCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    int count = 16;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        count = args[0].As<v8::Number>()->Value();
    }
    if (count < 0) {
        count = 0;
    }
    if (count > 1024 * 1024) {
        count = 1024 * 1024;
    }

    const std::string hex = BytesToHex(GenerateRandomBytes(static_cast<size_t>(count)));
    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), hex.c_str()).ToLocalChecked());
}

void HashCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    const std::string input = args.Length() > 0 ? ValueToString(isolate, args[0]) : std::string();
    std::string algorithm = "sha256";
    if (args.Length() > 1 && args[1]->IsString()) {
        algorithm = ValueToString(isolate, args[1]);
    }

    std::string hash = HashDigestHex(input, algorithm);
    if (hash.empty()) {
        hash = HashDigestHex(input, "sha256");
    }
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, hash.c_str()).ToLocalChecked());
}

void Base64EncodeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    const std::string input = args.Length() > 0 ? ValueToString(isolate, args[0]) : std::string();
    const std::string encoded = Base64Encode(input);
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, encoded.c_str()).ToLocalChecked());
}

void Base64DecodeCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    const std::string input = args.Length() > 0 ? ValueToString(isolate, args[0]) : std::string();
    const std::string decoded = Base64Decode(input);
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(isolate, decoded.data(), v8::NewStringType::kNormal, decoded.size())
            .ToLocalChecked());
}

}  // namespace

namespace modules {

bool RegisterCryptoModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> crypto = v8::Object::New(isolate);
    bool ok = crypto
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "randomBytes"),
                        v8::Function::New(context, RandomBytesCallback).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && crypto
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "randomHex"),
                         v8::Function::New(context, RandomHexCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && crypto
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "hash"),
                         v8::Function::New(context, HashCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && crypto
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "base64Encode"),
                         v8::Function::New(context, Base64EncodeCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && crypto
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "base64Decode"),
                         v8::Function::New(context, Base64DecodeCallback).ToLocalChecked())
                   .FromMaybe(false);

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Crypto"), crypto)
        .FromMaybe(false);
}

}  // namespace modules
