#include "modules/module_bridge_common.h"
#include "modules/module_builders.h"

#include "wrapper/cudnn/cudnn_engine_bridge.h"

namespace modules::detail {

bool BuildCudnnModule(v8::Isolate* isolate,
			  v8::Local<v8::Context> context,
			  v8::Local<v8::Object>* module_out,
			  std::string* error_out) {
	return BuildBridgeStatusModule(
		isolate,
		context,
		"CUDNN",
		engine::bridge::cudnn::IsAvailable(),
		engine::bridge::cudnn::Summary(),
		module_out,
		error_out);
}

}  // namespace modules::detail