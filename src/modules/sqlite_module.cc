#include "modules/sqlite_module.h"

#include <string>

#ifndef ENGINE_HAS_SQLITE3
#define ENGINE_HAS_SQLITE3 0
#endif

#if ENGINE_HAS_SQLITE3
#include <sqlite3.h>
#endif

namespace {

std::string ValueToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8(isolate, value);
    if (*utf8 == nullptr) {
        return std::string();
    }
    return *utf8;
}

void IsAvailableCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::Boolean::New(args.GetIsolate(), ENGINE_HAS_SQLITE3 == 1));
}

#if ENGINE_HAS_SQLITE3

void VersionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(args.GetIsolate(), sqlite3_libversion()).ToLocalChecked());
}

void ExecCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "exec expects (dbPath, sql)")));
        return;
    }

    const std::string db_path = ValueToString(isolate, args[0]);
    const std::string sql = ValueToString(isolate, args[1]);

    sqlite3* db = nullptr;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        const char* msg = db != nullptr ? sqlite3_errmsg(db) : "sqlite open failed";
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, msg).ToLocalChecked()));
        if (db != nullptr) {
            sqlite3_close(db);
        }
        return;
    }

    char* err_msg = nullptr;
    const int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        const char* msg = err_msg != nullptr ? err_msg : sqlite3_errmsg(db);
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, msg).ToLocalChecked()));
        if (err_msg != nullptr) {
            sqlite3_free(err_msg);
        }
        sqlite3_close(db);
        return;
    }

    sqlite3_close(db);
    args.GetReturnValue().Set(v8::Boolean::New(isolate, true));
}

void ScalarCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    v8::Isolate* isolate = args.GetIsolate();
    if (args.Length() < 2 || !args[0]->IsString() || !args[1]->IsString()) {
        isolate->ThrowException(v8::Exception::TypeError(
            v8::String::NewFromUtf8Literal(isolate, "scalar expects (dbPath, sql)")));
        return;
    }

    const std::string db_path = ValueToString(isolate, args[0]);
    const std::string sql = ValueToString(isolate, args[1]);

    sqlite3* db = nullptr;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        const char* msg = db != nullptr ? sqlite3_errmsg(db) : "sqlite open failed";
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, msg).ToLocalChecked()));
        if (db != nullptr) {
            sqlite3_close(db);
        }
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        const char* msg = sqlite3_errmsg(db);
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, msg).ToLocalChecked()));
        sqlite3_close(db);
        return;
    }

    const int step_rc = sqlite3_step(stmt);
    if (step_rc == SQLITE_ROW) {
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if (text == nullptr) {
            args.GetReturnValue().Set(v8::Null(isolate));
        } else {
            args.GetReturnValue().Set(
                v8::String::NewFromUtf8(isolate, reinterpret_cast<const char*>(text)).ToLocalChecked());
        }
    } else if (step_rc == SQLITE_DONE) {
        args.GetReturnValue().Set(v8::Null(isolate));
    } else {
        const char* msg = sqlite3_errmsg(db);
        isolate->ThrowException(v8::Exception::Error(
            v8::String::NewFromUtf8(isolate, msg).ToLocalChecked()));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

#else

void VersionCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(v8::String::NewFromUtf8Literal(args.GetIsolate(), "unavailable"));
}

void ExecCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetIsolate()->ThrowException(v8::Exception::Error(
        v8::String::NewFromUtf8Literal(args.GetIsolate(), "SQLite support is not available in this build")));
}

void ScalarCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetIsolate()->ThrowException(v8::Exception::Error(
        v8::String::NewFromUtf8Literal(args.GetIsolate(), "SQLite support is not available in this build")));
}

#endif

}  // namespace

namespace modules {

bool RegisterSQLiteModule(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Object> mod = v8::Object::New(isolate);
    bool ok = mod
                  ->Set(context,
                        v8::String::NewFromUtf8Literal(isolate, "isAvailable"),
                        v8::Function::New(context, IsAvailableCallback).ToLocalChecked())
                  .FromMaybe(false);
    ok = ok && mod
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "version"),
                         v8::Function::New(context, VersionCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && mod
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "exec"),
                         v8::Function::New(context, ExecCallback).ToLocalChecked())
                   .FromMaybe(false);
    ok = ok && mod
                   ->Set(context,
                         v8::String::NewFromUtf8Literal(isolate, "scalar"),
                         v8::Function::New(context, ScalarCallback).ToLocalChecked())
                   .FromMaybe(false);

    if (!ok) {
        return false;
    }

    return context->Global()
        ->Set(context, v8::String::NewFromUtf8Literal(isolate, "SQLite"), mod)
        .FromMaybe(false);
}

}  // namespace modules
