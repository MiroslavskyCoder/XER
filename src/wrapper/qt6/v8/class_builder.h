#pragma once

#include <string>
#include <vector>
#include <v8.h>

namespace qt6::v8bridge {

// ---------------------------------------------------------------------------
// Method definition helper
// ---------------------------------------------------------------------------
struct MethodDef {
    const char*           name;
    v8::FunctionCallback  fn;
};

struct AccessorDef {
    const char*           name;
    v8::AccessorNameGetterCallback getter;
    v8::AccessorNameSetterCallback setter = nullptr;  // null = read-only
};

// ---------------------------------------------------------------------------
// MakeClass
//   Creates a v8::FunctionTemplate representing a JS class.
//   internal_fields: number of C++ pointer slots per instance (usually 1).
// ---------------------------------------------------------------------------
v8::Local<v8::FunctionTemplate> MakeClass(
    v8::Isolate*                     isolate,
    const char*                      class_name,
    v8::FunctionCallback             constructor,
    const std::vector<MethodDef>&    instance_methods,
    const std::vector<MethodDef>&    static_methods = {},
    const std::vector<AccessorDef>&  accessors      = {},
    int                              internal_fields = 1);

// ---------------------------------------------------------------------------
// ExportClass
//   Installs the class constructor into the global context so JS can call
//   `new ClassName(...)`.
// ---------------------------------------------------------------------------
bool ExportClass(v8::Isolate*                     isolate,
                 v8::Local<v8::Context>            context,
                 const char*                       class_name,
                 v8::Local<v8::FunctionTemplate>   tpl);

// ---------------------------------------------------------------------------
// Object wrapping helpers
// ---------------------------------------------------------------------------

// Store a raw C++ pointer in internal field 0 of a V8 object.
// The object MUST have been created from a template with internal_fields >= 1.
template <typename T>
void WrapPointer(v8::Local<v8::Object> holder, T* ptr) {
    holder->SetInternalField(0, v8::External::New(holder->GetIsolate(), ptr));
}

// Retrieve the C++ pointer from internal field 0.
// Returns nullptr if the object is empty or the field is unset.
template <typename T>
T* UnwrapPointer(v8::Local<v8::Object> holder) {
    if (holder.IsEmpty() || holder->InternalFieldCount() < 1) return nullptr;
    auto field = holder->GetInternalField(0);
    if (field.IsEmpty() || !field->IsExternal()) return nullptr;
    return static_cast<T*>(field.As<v8::External>()->Value());
}

// ---------------------------------------------------------------------------
// GC-aware wrap: registers a weak persistent so ~T() is called on GC.
// Call once per constructed instance AFTER WrapPointer().
// ---------------------------------------------------------------------------
template <typename T>
void RegisterWeakCleanup(v8::Isolate* isolate, v8::Local<v8::Object> holder, T* ptr) {
    struct WeakData {
        T* object;
    };
    auto* data   = new WeakData{ ptr };
    auto* pst    = new v8::Persistent<v8::Object>(isolate, holder);
    pst->SetWeak(data, [](const v8::WeakCallbackInfo<WeakData>& info) {
        delete info.GetParameter()->object;
        delete info.GetParameter();
    }, v8::WeakCallbackType::kParameter);
    (void)pst;  // pst self-destructs after GC clears it
}

// ---------------------------------------------------------------------------
// Throw helpers (convenience)
// ---------------------------------------------------------------------------
inline void ThrowTypeError(v8::Isolate* iso, const char* msg) {
    iso->ThrowException(v8::Exception::TypeError(
        v8::String::NewFromUtf8(iso, msg).ToLocalChecked()));
}

inline void ThrowError(v8::Isolate* iso, const char* msg) {
    iso->ThrowException(v8::Exception::Error(
        v8::String::NewFromUtf8(iso, msg).ToLocalChecked()));
}

inline void ThrowError(v8::Isolate* iso, const std::string& msg) {
    iso->ThrowException(v8::Exception::Error(
        v8::String::NewFromUtf8(iso, msg.c_str()).ToLocalChecked()));
}

// ---------------------------------------------------------------------------
// Value conversion helpers
// ---------------------------------------------------------------------------
inline v8::Local<v8::String> ToV8Str(v8::Isolate* iso, const std::string& s) {
    return v8::String::NewFromUtf8(iso, s.c_str()).ToLocalChecked();
}

inline v8::Local<v8::String> ToV8Str(v8::Isolate* iso, const char* s) {
    return v8::String::NewFromUtf8(iso, s).ToLocalChecked();
}

inline std::string FromV8Str(v8::Isolate* iso, v8::Local<v8::Value> val) {
    v8::String::Utf8Value utf8(iso, val);
    return *utf8 ? *utf8 : "";
}

}  // namespace qt6::v8bridge
