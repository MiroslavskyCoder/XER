#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PXER_PLUGIN_API_VERSION 1u

typedef struct PxerPluginHandle PxerPluginHandle;

typedef struct PxerOwnedString {
	char* data;
	size_t size;
} PxerOwnedString;

typedef struct PxerPluginDescriptor {
	uint32_t api_version;
	const char* id;
	const char* name;
	const char* version;
	const char* description;
	const char* author;
} PxerPluginDescriptor;

typedef void (*PxerHostLogFn)(PxerPluginHandle* handle, int level, const char* message);
typedef int (*PxerHostSetValueFn)(PxerPluginHandle* handle, const char* key, const char* value);
typedef const char* (*PxerHostGetValueFn)(PxerPluginHandle* handle, const char* key, const char* fallback);
typedef int (*PxerHostRemoveValueFn)(PxerPluginHandle* handle, const char* key);
typedef int (*PxerHostRegisterCallFn)(PxerPluginHandle* handle, const char* call_name);
typedef int (*PxerHostSubscribeEventFn)(PxerPluginHandle* handle, const char* topic);
typedef int (*PxerHostEmitEventFn)(PxerPluginHandle* handle, const char* topic, const char* payload);
typedef uint64_t (*PxerHostScheduleAsyncFn)(PxerPluginHandle* handle, const char* task_name, const char* payload);


typedef int (*PxerHostRegisterJsModuleFn)(PxerPluginHandle* handle, const char* module_name, void* builder_fn_ptr);
typedef int (*PxerHostRegisterFluxTypeFn)(PxerPluginHandle* handle, const char* type_name, void* type_info_ptr);
typedef int (*PxerHostRegisterFluxTriggerFn)(PxerPluginHandle* handle, const char* trigger_name, void* trigger_info_ptr);
typedef int (*PxerHostRegisterGlobalHookFn)(PxerPluginHandle* handle, const char* hook_name, void* hook_fn_ptr);

typedef struct PxerHostApi {
	uint32_t api_version;
	PxerHostLogFn log;
	PxerHostSetValueFn set_setting;
	PxerHostGetValueFn get_setting;
	PxerHostRemoveValueFn remove_setting;
	PxerHostSetValueFn set_config;
	PxerHostGetValueFn get_config;
	PxerHostRemoveValueFn remove_config;
	PxerHostRegisterCallFn register_call;
	PxerHostRegisterCallFn unregister_call;
	PxerHostSubscribeEventFn subscribe_event;
	PxerHostSubscribeEventFn unsubscribe_event;
	PxerHostEmitEventFn emit_event;
	PxerHostScheduleAsyncFn schedule_async;

	// PXER EXTENSIONS FOR FULL CONTROL:
	PxerHostRegisterJsModuleFn register_js_module;
	PxerHostRegisterFluxTypeFn register_flux_type;
	PxerHostRegisterFluxTriggerFn register_flux_trigger;
	PxerHostRegisterGlobalHookFn register_global_hook;
} PxerHostApi;

typedef int (*PxerPluginLifecycleFn)(const PxerHostApi* host, PxerPluginHandle* handle);
typedef int (*PxerPluginInvokeFn)(const PxerHostApi* host,
				      PxerPluginHandle* handle,
				      const char* call_name,
				      const char* payload,
				      PxerOwnedString* out_result);
typedef int (*PxerPluginEventFn)(const PxerHostApi* host,
				     PxerPluginHandle* handle,
				     const char* topic,
				     const char* payload);
typedef int (*PxerPluginAsyncFn)(const PxerHostApi* host,
				     PxerPluginHandle* handle,
				     uint64_t task_id,
				     const char* task_name,
				     const char* payload,
				     PxerOwnedString* out_result);
typedef void (*PxerPluginReleaseStringFn)(PxerOwnedString* value);

typedef struct PxerPluginApi {
	const PxerPluginDescriptor* descriptor;
	PxerPluginLifecycleFn on_load;
	PxerPluginLifecycleFn on_unload;
	PxerPluginInvokeFn invoke;
	PxerPluginEventFn on_event;
	PxerPluginAsyncFn on_async;
	PxerPluginReleaseStringFn release_string;
} PxerPluginApi;

typedef const PxerPluginApi* (*PxerGetPluginApiFn)(void);

#ifdef __cplusplus
}
#endif