#include "modules/module_bridge_common.h"

namespace modules::detail {

bool SetBridgeStatusProperties(v8::Isolate* isolate,
			       v8::Local<v8::Context> context,
			       v8::Local<v8::Object> module,
			       const char* name,
			       bool available,
			       const std::string& summary,
			       std::string* error_out) {
	const bool ok = SetProperty(isolate, context, module, "name", Engine::Helper::ToV8Str(isolate, name))
		&& SetProperty(isolate, context, module, "available", v8::Boolean::New(isolate, available))
		&& SetProperty(isolate, context, module, "summary", Engine::Helper::ToV8Str(isolate, summary));
	if (!ok && error_out != nullptr) {
		*error_out = std::string("failed to build module: ") + name;
	}
	return ok;
}

bool BuildBridgeStatusModule(v8::Isolate* isolate,
			     v8::Local<v8::Context> context,
			     const char* name,
			     bool available,
			     const std::string& summary,
			     v8::Local<v8::Object>* module_out,
			     std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	if (!SetBridgeStatusProperties(isolate, context, module, name, available, summary, error_out)) {
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail