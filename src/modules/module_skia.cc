#include "modules/module_bridge_common.h"
#include "modules/module_builders.h"

#include "wrapper/skia/skia_engine_bridge.h"

namespace modules::detail {

bool BuildSkiaModule(v8::Isolate* isolate,
			 v8::Local<v8::Context> context,
			 v8::Local<v8::Object>* module_out,
			 std::string* error_out) {
	v8::Local<v8::Object> module = v8::Object::New(isolate);
	bool ok = SetBridgeStatusProperties(
		isolate,
		context,
		module,
		"Skia",
		engine::bridge::skia::IsAvailable(),
		engine::bridge::skia::Summary(),
		error_out);
	ok = ok && SetProperty(isolate, context, module, "version", Engine::Helper::ToV8Str(isolate, engine::bridge::skia::Version()));
	ok = ok && SetProperty(
		isolate,
		context,
		module,
		"exportedFunctions",
		MakeStringArray(isolate, context, engine::bridge::skia::ExportedFunctionNames()));
	ok = ok && SetProperty(
		isolate,
		context,
		module,
		"blendModes",
		MakeStringArray(isolate, context, engine::bridge::skia::BlendModeNames()));
	if (!ok) {
		if (error_out != nullptr && error_out->empty()) {
			*error_out = "failed to build Skia module";
		}
		return false;
	}
	*module_out = module;
	return true;
}

}  // namespace modules::detail