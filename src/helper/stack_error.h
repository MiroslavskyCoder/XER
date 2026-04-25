#pragma once

#include <v8.h>

#include <string>
#include <string_view>


namespace Engine::Helper {

class StackError {
public:
    static std::string BuildV8Report(v8::Isolate* isolate,
                                     v8::Local<v8::Context> context,
                                     const v8::TryCatch& try_catch,
                                     std::string_view phase,
                                     std::string_view source_text);
};

}