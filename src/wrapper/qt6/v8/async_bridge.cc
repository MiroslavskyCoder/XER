#include "wrapper/qt6/v8/async_bridge.h"

#include "async_io/io_thread_pool.h"

namespace qt6::async {

void DispatchInterrupt(v8::Isolate* isolate, void* data) {
    auto* state = static_cast<AsyncStateBase*>(data);
    state->Invoke(isolate);
    delete state;
}

void PostAsyncVoid(
    v8::Isolate*            isolate,
    v8::Local<v8::Function> callback,
    v8::Local<v8::Context>  context,
    std::function<bool(std::string& out_error)> work)
{
    PostAsync<bool>(
        isolate, callback, context,
        [work](std::string& err) { return work(err); },
        [](v8::Isolate* iso, const bool& ok) -> v8::Local<v8::Value> {
            return v8::Boolean::New(iso, ok);
        });
}

}  // namespace qt6::async
