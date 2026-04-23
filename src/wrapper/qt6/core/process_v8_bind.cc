#include "wrapper/qt6/core/process.h"

#include "wrapper/qt6/v8/class_builder.h"

namespace qt6::core::process_v8_detail {

void ProcCtor(const v8::FunctionCallbackInfo<v8::Value>& args);
void ProcSetWorkDir(const v8::FunctionCallbackInfo<v8::Value>& a);
void ProcSetEnv(const v8::FunctionCallbackInfo<v8::Value>& a);
void ProcClearEnv(const v8::FunctionCallbackInfo<v8::Value>& a);
void ProcRun(const v8::FunctionCallbackInfo<v8::Value>& a);
void ProcRunAsync(const v8::FunctionCallbackInfo<v8::Value>& a);

}  // namespace qt6::core::process_v8_detail

namespace qt6::core {

using namespace qt6::v8bridge;

bool RegisterQtProcessClass(v8::Isolate* isolate, v8::Local<v8::Context> context) {
    auto tpl = MakeClass(isolate, "QtProcess", process_v8_detail::ProcCtor,
        {
            { "setWorkDir", process_v8_detail::ProcSetWorkDir },
            { "setEnv", process_v8_detail::ProcSetEnv },
            { "clearEnv", process_v8_detail::ProcClearEnv },
            { "run", process_v8_detail::ProcRun },
            { "runAsync", process_v8_detail::ProcRunAsync },
        });

    return ExportClass(isolate, context, "QtProcess", tpl);
}

}  // namespace qt6::core
