#include "flow_script.h"

#include <libplatform/libplatform.h>

#include <cstdlib>
#include <mutex>
#include <utility>

#include <absl/strings/str_cat.h>

#include "error_handler/err_capture.h"
#include "flow_script_buffer.h"
#include "engine_params.h"
#include "flow_script_console.h"
#include "flow_script_event_bus.h"
#include "flow_script_import_module.h"
#include "flow_script_require.h"
#include "flux/terminal/terminal_output_renderer.h"
#include "resource_guard.h"

namespace {

std::mutex g_v8_mutex;
bool g_v8_initialized = false;
std::unique_ptr<v8::Platform> g_v8_platform;

void WriteFlowScriptLine(flux::terminal::OutputStream stream, const std::string& text) {
    flux::terminal::WriteLine(stream, text);
}

}  // namespace

void FlowScript::Init(v8::Isolate* isolate) {
    (void)isolate;

    std::lock_guard<std::mutex> guard(g_v8_mutex);
    if (g_v8_initialized) {
        return;
    }

    v8::V8::InitializeICUDefaultLocation(nullptr);
    v8::V8::InitializeExternalStartupData(nullptr);
    // Allow ENGINE_V8_PLATFORM_WORKERS to control the V8 background thread pool.
    {
        int v8_workers = 0;
        const char* raw = std::getenv("ENGINE_V8_PLATFORM_WORKERS");
        if (raw != nullptr && raw[0] != '\0') {
            try { v8_workers = std::stoi(raw); } catch (...) {}
        }
        g_v8_platform = v8::platform::NewDefaultPlatform(
            v8_workers > 0 ? v8_workers : 0  // 0 = use V8 default (hardware_concurrency)
        );
    }
    v8::V8::InitializePlatform(g_v8_platform.get());
    v8::V8::Initialize();
    g_v8_initialized = true;
}

void FlowScript::Shutdown() {
    std::lock_guard<std::mutex> guard(g_v8_mutex);
    if (!g_v8_initialized) {
        return;
    }

    v8::V8::Dispose();
    v8::V8::DisposePlatform();
    g_v8_platform.reset();
    g_v8_initialized = false;
}

v8::Isolate* FlowScript::CreateIsolate() {
    Init(nullptr);

    const EngineParams params = EngineParamsFromEnv();

    auto* allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = allocator;

    // Apply soft memory cap: ENGINE_MAX_MEMORY_USED (MiB) → set as V8 flag.
    // v8::ResourceConstraints::set_max_old_space_size was removed in newer V8;
    // use the --max-old-space-size flag string instead.
    if (params.max_memory_used > 0) {
        const std::string flag = "--max-old-space-size=" + std::to_string(params.max_memory_used);
        v8::V8::SetFlagsFromString(flag.c_str(), flag.size());
    }

    // Apply OS-level memory limit before allocating the isolate.
    // memory_hard_limit_mib overrides max_memory_used when explicitly set.
    const int hard_mib = params.memory_hard_limit_mib > 0
                         ? params.memory_hard_limit_mib
                         : params.max_memory_used;
    {
        std::string limit_error;
        ApplyProcessMemoryLimits(hard_mib, &limit_error);
        if (!limit_error.empty()) {
            WriteFlowScriptLine(flux::terminal::OutputStream::kStderr,
                absl::StrCat("[flow_script] ", limit_error));
        }
    }

    return v8::Isolate::New(create_params);
}

void FlowScript::DisposeIsolate(v8::Isolate* isolate) {
    if (isolate == nullptr) {
        return;
    }

    auto* allocator = isolate->GetArrayBufferAllocator();
    Engine::ErrorHandler::BindIsolate(nullptr);
    isolate->Dispose();
    delete allocator;
}

FlowScript::FlowScript(std::string path) : path_(std::move(path)), env_(nullptr) {}

FlowScriptEnv* FlowScript::create_env() {
    if (!env_) {
        auto data = std::make_unique<FlowScriptEnvData>();
        data->env["script_path"] = path_;
        env_ = std::make_unique<FlowScriptEnv>(data.get());
    }

    return env_.get();
}

bool FlowScript::Run() const {
    v8::Isolate* isolate = CreateIsolate();
    if (isolate == nullptr) {
        WriteFlowScriptLine(flux::terminal::OutputStream::kStderr, "Failed to create V8 isolate");
        return false;
    }
    Engine::ErrorHandler::BindIsolate(isolate);

    const EngineParams params = EngineParamsFromEnv();

    if (params.is_verbose()) {
        WriteFlowScriptLine(flux::terminal::OutputStream::kStderr, absl::StrCat("[flow_script] run: ", path_));
        if (params.sandbox) {
            WriteFlowScriptLine(flux::terminal::OutputStream::kStderr, "[flow_script]   sandbox mode enabled");
        }
        if (params.max_memory_used > 0) {
            WriteFlowScriptLine(flux::terminal::OutputStream::kStderr,
                absl::StrCat("[flow_script]   max_memory_used=", params.max_memory_used, " MiB"));
        }
        if (params.noemit) {
            WriteFlowScriptLine(flux::terminal::OutputStream::kStderr, "[flow_script]   noemit (dry-run)");
        }
    }

    bool success = false;
    {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = v8::Context::New(isolate);
        v8::Context::Scope context_scope(context);
        flow_script_detail::JsEventBusState event_bus_state;
        event_bus_state.runtime.Attach(isolate, context);

        do {
            // In sandbox mode, skip ImportModule binding (no dynamic module loading).
            if (!params.sandbox) {
                bool import_bound = context->Global()
                                        ->Set(
                                            context,
                                            v8::String::NewFromUtf8Literal(isolate, "ImportModule"),
                                            v8::Function::New(context, flow_script_detail::ImportModuleCallback)
                                                .ToLocalChecked())
                                        .FromMaybe(false);
                if (!import_bound) {
					WriteFlowScriptLine(flux::terminal::OutputStream::kStderr, "Failed to bind ImportModule");
                    break;
                }
            } else if (params.is_verbose()) {
				WriteFlowScriptLine(flux::terminal::OutputStream::kStderr,
					"[flow_script]   sandbox: ImportModule binding skipped");
            }

            if (!flow_script_detail::BindConsoleGlobals(isolate, context)) {
				WriteFlowScriptLine(flux::terminal::OutputStream::kStderr, "Failed to bind console");
                break;
            }

            if (!flow_script_detail::BindBuffer(isolate, context)) {
				WriteFlowScriptLine(flux::terminal::OutputStream::kStderr, "Failed to bind Buffer");
				break;
			}

            if (!flow_script_detail::BindEventBus(isolate, context, &event_bus_state)) {
				WriteFlowScriptLine(flux::terminal::OutputStream::kStderr, "Failed to bind EventBus");
                break;
            }

            flow_script_detail::FlowScriptRequireRuntime require_runtime(isolate, context, path_);
            if (!require_runtime.BindGlobals()) {
				WriteFlowScriptLine(flux::terminal::OutputStream::kStderr,
					"Failed to bind RequireFile/MakeExportModule APIs");
                break;
            }

            std::string run_error;
            // Start resource monitor: enforces memory hard limit and timeout.
            ResourceGuard guard(isolate, ResourceGuardOptionsFromEnv());
            guard.Start();

            const bool run_ok = require_runtime.RunEntry(&run_error);

            guard.Stop();

            if (!run_ok) {
                if (guard.was_killed() && !guard.kill_reason().empty()) {
                    WriteFlowScriptLine(flux::terminal::OutputStream::kStderr,
                        absl::StrCat("[resource_guard] script aborted: ", guard.kill_reason()));
                } else {
                    WriteFlowScriptLine(flux::terminal::OutputStream::kStderr, run_error);
                }
                break;
            }

            success = true;
        } while (false);
    }

    DisposeIsolate(isolate);
    return success;
}
