#include "modules/module_bridge_common.h"
#include "modules/module_builders.h"

namespace modules::detail {

bool BuildAiModule(v8::Isolate* isolate,
			v8::Local<v8::Context> context,
			v8::Local<v8::Object>* module_out,
			std::string* error_out) {
	return BuildBridgeStatusModule(
		isolate,
		context,
		"AI",
		false,
		"AI module disabled by ENABLE_AI=OFF",
		module_out,
		error_out);
}

bool BuildSdModule(v8::Isolate* isolate,
			v8::Local<v8::Context> context,
			v8::Local<v8::Object>* module_out,
			std::string* error_out) {
	return BuildBridgeStatusModule(
		isolate,
		context,
		"SD",
		false,
		"SD module disabled by ENABLE_AI=OFF",
		module_out,
		error_out);
}

}  // namespace modules::detail
