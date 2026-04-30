// Minimal PXER plugin: registers JS module, Flux type, and global hook
#include "pxer_plugin_api.h"
#include <stdio.h>
#include <string.h>

// --- JS module builder stub ---
static int ExampleJsModuleBuilder(void* v8_isolate, void* v8_context, void* module_out, void* error_out) {
    // In real plugin: fill module_out with V8 object
    (void)v8_isolate; (void)v8_context; (void)module_out; (void)error_out;
    printf("[pxer_example_plugin] JS module builder called\n");
    return 1;
}

// --- Flux type info stub ---
typedef struct ExampleFluxTypeInfo {
    int dummy;
} ExampleFluxTypeInfo;

// --- Flux trigger info stub ---
typedef struct ExampleFluxTriggerInfo {
    int dummy;
} ExampleFluxTriggerInfo;

// --- Global hook stub ---
static int ExampleGlobalHook(const char* event, void* data) {
    printf("[pxer_example_plugin] Global hook: %s\n", event);
    (void)data;
    return 0;
}

static const PxerPluginDescriptor descriptor = {
    PXER_PLUGIN_API_VERSION,
    "pxer_example_plugin",
    "PXER Example Plugin",
    "1.0.0",
    "Demo plugin: registers JS module, Flux type, and hook",
    "copilot"
};

static int on_load(const PxerHostApi* host, PxerPluginHandle* handle) {
    // Register JS module
    if (host->register_js_module) {
        host->register_js_module(handle, "ExampleJsModule", (void*)&ExampleJsModuleBuilder);
    }
    // Register Flux type
    if (host->register_flux_type) {
        static ExampleFluxTypeInfo type_info = {0};
        host->register_flux_type(handle, "ExampleType", (void*)&type_info);
    }
    // Register Flux trigger
    if (host->register_flux_trigger) {
        static ExampleFluxTriggerInfo trigger_info = {0};
        host->register_flux_trigger(handle, "ExampleTrigger", (void*)&trigger_info);
    }
    // Register global hook
    if (host->register_global_hook) {
        host->register_global_hook(handle, "ExampleGlobalHook", (void*)&ExampleGlobalHook);
    }
    if (host->log) host->log(handle, 1, "pxer_example_plugin loaded");
    return 0;
}

static int on_unload(const PxerHostApi* host, PxerPluginHandle* handle) {
    if (host->log) host->log(handle, 1, "pxer_example_plugin unloaded");
    return 0;
}

static int invoke(const PxerHostApi* host, PxerPluginHandle* handle, const char* call_name, const char* payload, PxerOwnedString* out_result) {
    (void)host; (void)handle; (void)call_name; (void)payload;
    out_result->data = strdup("{\"ok\":true}");
    out_result->size = strlen(out_result->data);
    return 0;
}

static void release_string(PxerOwnedString* value) {
    if (value && value->data) free(value->data);
    value->data = NULL;
    value->size = 0;
}

static const PxerPluginApi api = {
    &descriptor,
    &on_load,
    &on_unload,
    &invoke,
    NULL, // on_event
    NULL, // on_async
    &release_string
};

#ifdef _WIN32
__declspec(dllexport)
#endif
const PxerPluginApi* pxer_get_plugin_api(void) {
    return &api;
}
