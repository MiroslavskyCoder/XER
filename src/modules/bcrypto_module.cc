#include "modules/bcrypto_module.h"

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <iomanip>
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

std::string BytesToHex(const uint8_t* data, size_t size) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (size_t i = 0; i < size; ++i) {
        out << std::setw(2) << static_cast<int>(data[i]);
    }
    return out.str();
}

std::string DigestHex(const std::string& input, const EVP_MD* md) {
    if (md == nullptr) {
        return std::string();
    }

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        return std::string();
    }

    std::vector<uint8_t> digest(static_cast<size_t>(EVP_MD_get_size(md)));
    unsigned int digest_len = 0;
    const bool ok = EVP_DigestInit_ex(ctx, md, nullptr) == 1 &&
                    EVP_DigestUpdate(ctx, input.data(), input.size()) == 1 &&
                    EVP_DigestFinal_ex(ctx, digest.data(), &digest_len) == 1;
    EVP_MD_CTX_free(ctx);

    if (!ok) {
        return std::string();
    }

    return BytesToHex(digest.data(), digest_len);
}

void Sha256Callback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const std::string input = args.Length() > 0 ? ValueToString(args.GetIsolate(), args[0]) : std::string();
    const std::string out = DigestHex(input, EVP_sha256());
    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), out.c_str()).ToLocalChecked());
}

void Sha512Callback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const std::string input = args.Length() > 0 ? ValueToString(args.GetIsolate(), args[0]) : std::string();
    const std::string out = DigestHex(input, EVP_sha512());
    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), out.c_str()).ToLocalChecked());
}

void Blake2bCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    const std::string input = args.Length() > 0 ? ValueToString(args.GetIsolate(), args[0]) : std::string();
    const std::string out = DigestHex(input, EVP_blake2b512());
    args.GetReturnValue().Set(v8::String::NewFromUtf8(args.GetIsolate(), out.c_str()).ToLocalChecked());
}

void HmacSha256Callback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    const std::string key = args.Length() > 0 ? ValueToString(isolate, args[0]) : std::string();
    const std::string input = args.Length() > 1 ? ValueToString(isolate, args[1]) : std::string();

    unsigned int out_len = 0;
    uint8_t out[EVP_MAX_MD_SIZE];
    if (HMAC(EVP_sha256(),
             key.data(),
             static_cast<int>(key.size()),
             reinterpret_cast<const uint8_t*>(input.data()),
             input.size(),
             out,
             &out_len) == nullptr) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "HMAC-SHA256 failed")));
        return;
    }

    const std::string hex = BytesToHex(out, out_len);
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, hex.c_str()).ToLocalChecked());
}

void Pbkdf2Callback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    const std::string password = args.Length() > 0 ? ValueToString(isolate, args[0]) : std::string();
    const std::string salt = args.Length() > 1 ? ValueToString(isolate, args[1]) : std::string();

    int iterations = 100000;
    if (args.Length() > 2 && args[2]->IsNumber()) {
        iterations = args[2].As<v8::Number>()->Value();
    }
    if (iterations < 1) {
        iterations = 1;
    }

    int out_len = 32;
    if (args.Length() > 3 && args[3]->IsNumber()) {
        out_len = args[3].As<v8::Number>()->Value();
    }
    if (out_len < 16) {
        out_len = 16;
    }
    if (out_len > 128) {
        out_len = 128;
    }

    std::vector<uint8_t> out(static_cast<size_t>(out_len));
    if (PKCS5_PBKDF2_HMAC(password.data(),
                          static_cast<int>(password.size()),
                          reinterpret_cast<const uint8_t*>(salt.data()),
                          static_cast<int>(salt.size()),
                          iterations,
                          EVP_sha256(),
                          out_len,
                          out.data()) != 1) {
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8Literal(isolate, "PBKDF2 failed")));
        return;
    }

    const std::string hex = BytesToHex(out.data(), out.size());
    args.GetReturnValue().Set(v8::String::NewFromUtf8(isolate, hex.c_str()).ToLocalChecked());
}

}  // namespace

namespace modules {

bool RegisterBcryptoModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> mod = v8::Object::New(isolate);
    bool ok = mod
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "sha256"),
                        v8::Function::New(context, Sha256Callback).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && mod
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "sha512"),
                         v8::Function::New(context, Sha512Callback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && mod
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "blake2b"),
                         v8::Function::New(context, Blake2bCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && mod
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "hmacSha256"),
                         v8::Function::New(context, HmacSha256Callback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && mod
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "pbkdf2"),
                         v8::Function::New(context, Pbkdf2Callback).ToLocalChecked())
                   .FromMaybe(false);

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "Bcrypto"), mod)
        .FromMaybe(false);
}

}  // namespace modules
